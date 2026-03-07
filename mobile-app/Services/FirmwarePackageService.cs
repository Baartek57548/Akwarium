using System.Buffers.Binary;
using System.Text;
using Microsoft.Maui.Storage;

namespace AquariumController.Mobile.Services;

public interface IFirmwarePackageService
{
	Task<FirmwarePackageDescriptor?> PickFirmwarePackageAsync(CancellationToken cancellationToken = default);
}

public sealed class FirmwarePackageService : IFirmwarePackageService
{
	private const byte EspImageMagic = 0xE9;
	private const ushort Esp32S3ChipId = 0x0009;
	private const int EspImageHeaderSize = 24;
	private const int EspImageSegmentHeaderSize = 8;
	private const int EspAppDescriptionOffset = EspImageHeaderSize + EspImageSegmentHeaderSize;
	private const int EspAppDescriptionMagicOffset = EspAppDescriptionOffset;
	private const int EspAppDescriptionSize = 256;
	private const uint EspAppDescriptionMagic = 0xABCD5432;
	private static readonly byte[] MetadataPrefix = Encoding.ASCII.GetBytes("AQFWMETA|");

	private readonly IFilePicker _filePicker;

	public FirmwarePackageService(IFilePicker filePicker)
	{
		_filePicker = filePicker;
	}

	public async Task<FirmwarePackageDescriptor?> PickFirmwarePackageAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		var result = await _filePicker.PickAsync(new PickOptions
		{
			PickerTitle = "Wybierz pakiet firmware (.bin)",
			FileTypes = new FilePickerFileType(new Dictionary<DevicePlatform, IEnumerable<string>>
			{
				[DevicePlatform.Android] = ["application/octet-stream", "application/bin", ".bin"],
				[DevicePlatform.WinUI] = [".bin"]
			})
		});

		if (result is null)
		{
			return null;
		}

		cancellationToken.ThrowIfCancellationRequested();

		await using var sourceStream = await result.OpenReadAsync();
		using var buffer = new MemoryStream();
		await sourceStream.CopyToAsync(buffer, cancellationToken);
		var bytes = buffer.ToArray();
		var metadata = ParseMetadata(bytes, result.FileName);
		return new FirmwarePackageDescriptor(result.FileName, bytes, metadata);
	}

	private static FirmwarePackageMetadata ParseMetadata(byte[] bytes, string fileName)
	{
		var metadata = new FirmwarePackageMetadata
		{
			FileName = fileName,
			FileSizeBytes = bytes.Length
		};

		if (bytes.Length < (EspAppDescriptionOffset + EspAppDescriptionSize))
		{
			return metadata with
			{
				ValidationMessage = "Plik jest zbyt mały, aby był prawidłowym obrazem firmware ESP32."
			};
		}

		var chipId = BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(12, 2));
		var isEspImage = bytes[0] == EspImageMagic;
		var isCompatibleWithEsp32S3 = chipId == Esp32S3ChipId;
		var appDescMagic = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(EspAppDescriptionMagicOffset, 4));
		var hasAppDescription = appDescMagic == EspAppDescriptionMagic;

		var buildDate = hasAppDescription
			? ReadAsciiField(bytes, EspAppDescriptionOffset + 96, 16)
			: "-";
		var buildTime = hasAppDescription
			? ReadAsciiField(bytes, EspAppDescriptionOffset + 80, 16)
			: "-";
		var idfVersion = hasAppDescription
			? ReadAsciiField(bytes, EspAppDescriptionOffset + 112, 32)
			: "-";

		var marker = TryReadMetadataMarker(bytes);
		var hasAquariumMetadata = !string.IsNullOrWhiteSpace(marker);
		var keyValues = hasAquariumMetadata
			? ParseMarker(marker!)
			: new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

		var projectName = keyValues.TryGetValue("project", out var project)
			? project
			: Path.GetFileNameWithoutExtension(fileName);
		var version = keyValues.TryGetValue("version", out var versionValue)
			? versionValue
			: (hasAppDescription ? ReadAsciiField(bytes, EspAppDescriptionOffset + 16, 32) : "unknown");

		if (keyValues.TryGetValue("date", out var markerDate) && !string.IsNullOrWhiteSpace(markerDate))
		{
			buildDate = markerDate;
		}

		if (keyValues.TryGetValue("time", out var markerTime) && !string.IsNullOrWhiteSpace(markerTime))
		{
			buildTime = markerTime;
		}

		if (keyValues.TryGetValue("idf", out var markerIdf) && !string.IsNullOrWhiteSpace(markerIdf))
		{
			idfVersion = markerIdf;
		}

		var validationMessage = !isEspImage
			? "Wybrany plik nie jest prawidłowym obrazem ESP32."
			: !isCompatibleWithEsp32S3
				? $"Firmware jest przeznaczony dla układu 0x{chipId:X4}; wymagany jest ESP32-S3."
				: hasAquariumMetadata
					? "Pakiet jest gotowy do aktualizacji BLE OTA."
					: "Wykryto obraz ESP32-S3, ale brakuje znacznika metadanych Aquarium.";

		return metadata with
		{
			ChipId = chipId,
			IsEspImage = isEspImage,
			IsCompatibleWithEsp32S3 = isCompatibleWithEsp32S3,
			HasAquariumMetadata = hasAquariumMetadata,
			ProjectName = string.IsNullOrWhiteSpace(projectName) ? "Aquarium Controller" : projectName,
			Version = string.IsNullOrWhiteSpace(version) ? "unknown" : version,
			BuildDate = string.IsNullOrWhiteSpace(buildDate) ? "-" : buildDate,
			BuildTime = string.IsNullOrWhiteSpace(buildTime) ? "-" : buildTime,
			IdfVersion = string.IsNullOrWhiteSpace(idfVersion) ? "-" : idfVersion,
			ValidationMessage = validationMessage
		};
	}

	private static string ReadAsciiField(byte[] bytes, int offset, int length)
	{
		return Encoding.ASCII.GetString(bytes, offset, length)
			.Split('\0', 2)[0]
			.Trim();
	}

	private static string? TryReadMetadataMarker(byte[] bytes)
	{
		var span = bytes.AsSpan();
		var start = span.IndexOf(MetadataPrefix);
		if (start < 0)
		{
			return null;
		}

		var end = start;
		while (end < bytes.Length && bytes[end] != 0)
		{
			end++;
		}

		return Encoding.ASCII.GetString(bytes, start, end - start);
	}

	private static Dictionary<string, string> ParseMarker(string marker)
	{
		var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
		foreach (var part in marker.Split('|', StringSplitOptions.RemoveEmptyEntries))
		{
			var separatorIndex = part.IndexOf('=');
			if (separatorIndex <= 0)
			{
				continue;
			}

			var key = part[..separatorIndex].Trim();
			var value = part[(separatorIndex + 1)..].Trim();
			if (key.Length > 0)
			{
				result[key] = value;
			}
		}

		return result;
	}
}

public sealed record FirmwarePackageDescriptor(string FileName, byte[] FirmwareImage, FirmwarePackageMetadata Metadata)
{
	public string FileSizeText => FirmwareImage.Length <= 0
		? "0 B"
		: $"{FirmwareImage.Length / 1024d / 1024d:0.00} MB";
}

public sealed record FirmwarePackageMetadata
{
	private const ushort Esp32S3ChipId = 0x0009;

	public string FileName { get; init; } = string.Empty;

	public int FileSizeBytes { get; init; }

	public ushort ChipId { get; init; }

	public bool IsEspImage { get; init; }

	public bool IsCompatibleWithEsp32S3 { get; init; }

	public bool HasAquariumMetadata { get; init; }

	public string ProjectName { get; init; } = "Aquarium Controller";

	public string Version { get; init; } = "unknown";

	public string BuildDate { get; init; } = "-";

	public string BuildTime { get; init; } = "-";

	public string IdfVersion { get; init; } = "-";

	public string ValidationMessage { get; init; } = string.Empty;

	public string ChipDisplayText => ChipId == 0
		? "-"
		: ChipId == Esp32S3ChipId
			? "ESP32-S3"
			: $"0x{ChipId:X4}";

	public string BuildDisplayText => $"{BuildDate} {BuildTime}".Trim();

	public bool CanBeUploaded => IsEspImage && IsCompatibleWithEsp32S3;
}

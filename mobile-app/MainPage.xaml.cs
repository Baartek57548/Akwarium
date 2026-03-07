using AquariumController.Mobile.ViewModels;

namespace AquariumController.Mobile;

public partial class MainPage : ContentPage
{
	private readonly MainViewModel _viewModel;
	private bool _initialized;

	public MainPage(MainViewModel viewModel)
	{
		InitializeComponent();
		BindingContext = _viewModel = viewModel;
	}

	protected override async void OnAppearing()
	{
		base.OnAppearing();

		if (_initialized)
		{
			return;
		}

		_initialized = true;
		await _viewModel.InitializeAsync();
	}
}

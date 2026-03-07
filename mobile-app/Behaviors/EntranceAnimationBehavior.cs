using System.ComponentModel;
using Microsoft.Maui.Controls;

namespace AquariumController.Mobile.Behaviors;

public sealed class EntranceAnimationBehavior : Behavior<VisualElement>
{
	public uint Duration { get; set; } = 220;

	public double OffsetY { get; set; } = 18d;

	private VisualElement? _element;
	private bool _isAnimating;

	protected override void OnAttachedTo(VisualElement bindable)
	{
		base.OnAttachedTo(bindable);
		_element = bindable;
		Prepare(bindable);
		bindable.Loaded += HandleLoaded;
		bindable.PropertyChanged += HandlePropertyChanged;
	}

	protected override void OnDetachingFrom(VisualElement bindable)
	{
		bindable.Loaded -= HandleLoaded;
		bindable.PropertyChanged -= HandlePropertyChanged;
		_element = null;
		base.OnDetachingFrom(bindable);
	}

	private void HandleLoaded(object? sender, EventArgs e)
	{
		_ = RunAnimationAsync();
	}

	private void HandlePropertyChanged(object? sender, PropertyChangedEventArgs e)
	{
		if (e.PropertyName == VisualElement.IsVisibleProperty.PropertyName && _element?.IsVisible == true)
		{
			Prepare(_element);
			_ = RunAnimationAsync();
		}
	}

	private void Prepare(VisualElement element)
	{
		element.Opacity = 0d;
		element.TranslationY = OffsetY;
	}

	private async Task RunAnimationAsync()
	{
		if (_element is null || !_element.IsVisible || _isAnimating)
		{
			return;
		}

		_isAnimating = true;
		try
		{
			await Task.WhenAll(
				_element.FadeToAsync(1d, Duration, Easing.CubicOut),
				_element.TranslateToAsync(0d, 0d, Duration, Easing.CubicOut));
		}
		finally
		{
			_isAnimating = false;
		}
	}
}

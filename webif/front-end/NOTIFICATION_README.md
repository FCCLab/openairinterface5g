# Notification System

This document describes the notification system implemented in the OpenAirInterface5G Web Interface.

## Overview

The notification system provides a modern, user-friendly way to display error messages, success notifications, warnings, and informational messages throughout the application. It replaces the basic browser `alert()` dialogs with styled, responsive popups that support retry actions and auto-close functionality. The popups appear in corners without masking the main screen, and multiple notifications stack from the bottom-right, with new notifications appearing on top and existing ones dropping down when notifications are removed, allowing users to continue working while notifications are displayed.

## Components

### 1. NotificationPopup Component (`src/components/NotificationPopup.js`)
The main popup component that renders the notification message with appropriate styling and actions.

**Features:**
- Multiple message types (error, success, warning, info)
- Corner positioning (top-right, top-left, bottom-right, bottom-left)
- Multiple notification stacking from bottom-right (new notifications appear on top)
- Smooth drop-down animation when notifications are removed
- Auto-close functionality with configurable delays (enabled by default)
- Non-intrusive design (doesn't mask the main screen)
- Retry action support
- Keyboard navigation (ESC to close, Enter to retry)
- Responsive design
- Smooth animations

### 2. NotificationContext (`src/context/NotificationContext.js`)
A React context that provides global notification management across the application.

**Provides:**
- `showError()` - Display error messages with optional retry action and corner positioning
- `showSuccess()` - Display success messages (auto-closes)
- `showWarning()` - Display warning messages with optional action
- `showInfo()` - Display informational messages (auto-closes)
- `hideError()` - Manually hide the current popup

## Usage

### Basic Usage

```javascript
import { useError } from '../context/ErrorContext';

function MyComponent() {
  const { showError, showSuccess, showWarning, showInfo } = useNotification();

  // Show a simple error message (auto-closes in 5 seconds)
  showError({ message: 'Something went wrong!' });

  // Show error in specific corner with custom auto-close
  showError({ 
    message: 'Error in top-left corner', 
    position: 'top-left',
    autoCloseDelay: 3000
  });

  // Show success message (auto-closes in 3 seconds)
  showSuccess('Operation completed successfully!');

  // Show warning with retry action
  showWarning('Please review your settings.', () => {
    console.log('Settings reviewed');
  });

  // Show info message (auto-closes in 4 seconds)
  showInfo('This is an informational message.');
}
```

### Error with Retry Action

```javascript
const handleNetworkError = () => {
  showError({
    message: 'Network connection failed. Please check your connection and try again.',
    onRetry: () => {
      // Retry the failed operation
      fetchData();
    }
  });
};
```

### Auto-Close Settings

```javascript
// Default behavior (auto-closes in 5 seconds)
showError({ message: 'This will auto-close in 5 seconds' });

// Custom auto-close timing
showError({
  message: 'This error will auto-close in 10 seconds',
  autoClose: true,
  autoCloseDelay: 10000
});

// Disable auto-close (for critical errors)
showError({
  message: 'Critical error - requires user attention',
  autoClose: false
});
```

### Corner Positioning

```javascript
// Available positions: 'top-right', 'top-left', 'bottom-right', 'bottom-left'
showError({
  message: 'Error in bottom-left corner',
  position: 'bottom-left'
});
```

## Message Types

### Error
- **Icon**: ❌
- **Color**: Red gradient
- **Auto-close**: Disabled by default
- **Use case**: Critical errors that require user attention

### Success
- **Icon**: ✅
- **Color**: Green gradient
- **Auto-close**: 3 seconds by default
- **Use case**: Successful operations

### Warning
- **Icon**: ⚠️
- **Color**: Orange gradient
- **Auto-close**: Disabled by default
- **Use case**: Warnings that may need user action

### Info
- **Icon**: ℹ️
- **Color**: Blue gradient
- **Auto-close**: 4 seconds by default
- **Use case**: Informational messages

## Integration

### 1. Wrap your app with NotificationProvider

```javascript
// In App.js
import { NotificationProvider } from './context/NotificationContext';

function App() {
  return (
    <NotificationProvider>
      {/* Your app components */}
    </NotificationProvider>
  );
}
```

### 2. Use in any component

```javascript
import { useNotification } from '../context/NotificationContext';

function MyComponent() {
  const { showError, showSuccess } = useNotification();

  const handleApiCall = async () => {
    try {
      const response = await fetch('/api/data');
      if (response.ok) {
        showSuccess('Data loaded successfully!');
      } else {
        showError({
          message: 'Failed to load data. Please try again.',
          onRetry: handleApiCall
        });
      }
    } catch (error) {
      showError({
        message: 'Network error. Please check your connection.',
        onRetry: handleApiCall
      });
    }
  };

  return (
    <button onClick={handleApiCall}>
      Load Data
    </button>
  );
}
```

## Styling

The popup system uses CSS modules and includes:
- Responsive design for mobile devices
- Dark theme support
- Smooth animations and transitions
- Backdrop blur effect
- Modern gradient backgrounds

## Accessibility

- Keyboard navigation support (ESC to close, Enter to retry)
- Screen reader friendly
- Focus management
- High contrast support

## Migration from Alert()

### Before (using alert)
```javascript
try {
  await fetchData();
  alert('Data loaded successfully!');
} catch (error) {
  alert('Error loading data: ' + error.message);
}
```

### After (using error popup)
```javascript
try {
  await fetchData();
  showSuccess('Data loaded successfully!');
} catch (error) {
  showError({
    message: 'Error loading data: ' + error.message,
    onRetry: fetchData
  });
}
```

## Demo

A demo component is available at `src/components/NotificationDemo.js` that showcases all the different types of popups and their features.

## Files Modified

1. `src/components/NotificationPopup.js` - Main popup component
2. `src/components/NotificationPopup.css` - Popup styles
3. `src/context/NotificationContext.js` - Notification context provider
4. `src/App.js` - Added NotificationProvider wrapper
5. `src/pages/HomePage.js` - Updated to use notifications
6. `src/components/SpectrogramContainer.js` - Replaced alerts with notifications
7. `src/pages/CellScanPage.js` - Added success notifications

## Benefits

1. **Better UX**: Modern, styled popups instead of basic browser alerts
2. **Non-Intrusive**: Doesn't block or mask the main screen
3. **Multiple Notifications**: Stack from bottom for better organization
4. **Consistent Design**: Unified error handling across the application
5. **Retry Actions**: Users can retry failed operations directly from the popup
6. **Auto-close**: Messages auto-close by default to reduce user interaction
7. **Responsive**: Works well on mobile and desktop devices
8. **Accessible**: Keyboard navigation and screen reader support
9. **Maintainable**: Centralized error handling logic

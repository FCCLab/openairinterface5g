import React from 'react';
import { useNotification } from '../context/NotificationContext';

const NotificationDemo = () => {
  const { showError, showSuccess, showWarning, showInfo } = useNotification();

  const handleErrorDemo = () => {
    showError({
      message: 'This is an example error message. Something went wrong with the operation.',
      onRetry: () => {
        console.log('Retry action triggered');
        showSuccess('Operation retried successfully!');
      },
      position: 'bottom-right',
      autoClose: true,
      autoCloseDelay: 5000
    });
  };

  const handleSuccessDemo = () => {
    showSuccess('Operation completed successfully! This message will auto-close in 3 seconds.');
  };

  const handleQuickMessage = () => {
    showError({
      message: 'Quick notification - auto-closes in 2 seconds',
      autoClose: true,
      autoCloseDelay: 2000,
      position: 'bottom-right'
    });
  };

  const handleWarningDemo = () => {
    showWarning('This is a warning message. Please review your settings before proceeding.', () => {
      console.log('Warning acknowledged');
      showInfo('Settings reviewed and confirmed.');
    });
  };

  const handleInfoDemo = () => {
    showInfo('This is an informational message. It will auto-close in 4 seconds.');
  };

  const handleNetworkError = () => {
    showError({
      message: 'Network connection failed. Please check your internet connection and try again.',
      onRetry: () => {
        console.log('Retrying network connection...');
        // Simulate retry
        setTimeout(() => {
          showSuccess('Network connection restored!');
        }, 1000);
      },
      position: 'bottom-right',
      autoClose: true,
      autoCloseDelay: 8000
    });
  };

  const handleDeviceError = () => {
    showError({
      message: 'USRP device not detected. Please ensure the device is properly connected and try again.',
      onRetry: () => {
        console.log('Retrying device detection...');
        // Simulate retry
        setTimeout(() => {
          showSuccess('USRP device detected successfully!');
        }, 1500);
      },
      position: 'top-left',
      autoClose: true,
      autoCloseDelay: 10000
    });
  };

  const handleTopLeftError = () => {
    showError({
      message: 'Top-left corner error message.',
      position: 'top-left',
      autoClose: true,
      autoCloseDelay: 4000
    });
  };

  const handleBottomLeftError = () => {
    showError({
      message: 'Bottom-left corner error message.',
      position: 'bottom-left',
      autoClose: true,
      autoCloseDelay: 4000
    });
  };

  const handleBottomRightError = () => {
    showError({
      message: 'Bottom-right corner error message.',
      position: 'bottom-right',
      autoClose: true,
      autoCloseDelay: 4000
    });
  };

  const handleMultipleNotifications = () => {
    // Show multiple notifications that will stack from bottom-right
    showError({
      message: 'First notification - appears at bottom-right',
      position: 'bottom-right',
      autoClose: true,
      autoCloseDelay: 6000
    });
    
    setTimeout(() => {
      showError({
        message: 'Second notification - stacks on top',
        position: 'bottom-right',
        autoClose: true,
        autoCloseDelay: 6000
      });
    }, 500);
    
    setTimeout(() => {
      showError({
        message: 'Third notification - continues stacking',
        position: 'bottom-right',
        autoClose: true,
        autoCloseDelay: 6000
      });
    }, 1000);
    
    setTimeout(() => {
      showError({
        message: 'Fourth notification - full stack visible',
        position: 'bottom-right',
        autoClose: true,
        autoCloseDelay: 6000
      });
    }, 1500);
  };

  return (
    <div className="demo-container" style={{
      padding: '2rem',
      maxWidth: '800px',
      margin: '0 auto',
      color: 'white'
    }}>
      <h2>Error Popup System Demo</h2>
      <p>Click the buttons below to see different types of error popups in action.</p>
      
      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fit, minmax(200px, 1fr))',
        gap: '1rem',
        marginTop: '2rem'
      }}>
        <button 
          onClick={handleErrorDemo}
          style={{
            padding: '1rem',
            backgroundColor: '#dc2626',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Show Error with Retry
        </button>
        
        <button 
          onClick={handleSuccessDemo}
          style={{
            padding: '1rem',
            backgroundColor: '#059669',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Show Success Message
        </button>
        
        <button 
          onClick={handleQuickMessage}
          style={{
            padding: '1rem',
            backgroundColor: '#0891b2',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Quick Message (2s)
        </button>
        
        <button 
          onClick={handleWarningDemo}
          style={{
            padding: '1rem',
            backgroundColor: '#d97706',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Show Warning with Action
        </button>
        
        <button 
          onClick={handleInfoDemo}
          style={{
            padding: '1rem',
            backgroundColor: '#2563eb',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Show Info Message
        </button>
        
        <button 
          onClick={handleNetworkError}
          style={{
            padding: '1rem',
            backgroundColor: '#7c3aed',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Simulate Network Error
        </button>
        
        <button 
          onClick={handleDeviceError}
          style={{
            padding: '1rem',
            backgroundColor: '#be185d',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Simulate Device Error
        </button>
        
        <button 
          onClick={handleTopLeftError}
          style={{
            padding: '1rem',
            backgroundColor: '#059669',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Top-Left Corner
        </button>
        
        <button 
          onClick={handleBottomLeftError}
          style={{
            padding: '1rem',
            backgroundColor: '#7c2d12',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Bottom-Left Corner
        </button>
        
        <button 
          onClick={handleBottomRightError}
          style={{
            padding: '1rem',
            backgroundColor: '#1e40af',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Bottom-Right Corner
        </button>
        
        <button 
          onClick={handleMultipleNotifications}
          style={{
            padding: '1rem',
            backgroundColor: '#dc2626',
            color: 'white',
            border: 'none',
            borderRadius: '8px',
            cursor: 'pointer',
            fontWeight: '600'
          }}
        >
          Multiple Stacking Notifications
        </button>
      </div>
      
      <div style={{
        marginTop: '2rem',
        padding: '1rem',
        backgroundColor: 'rgba(255, 255, 255, 0.1)',
        borderRadius: '8px'
      }}>
        <h3>Features:</h3>
        <ul>
          <li>✅ Different message types (Error, Success, Warning, Info)</li>
          <li>✅ Auto-close functionality with configurable delays</li>
          <li>✅ Retry actions for error recovery</li>
          <li>✅ Responsive design for mobile devices</li>
          <li>✅ Smooth animations and transitions</li>
          <li>✅ Click outside to close</li>
          <li>✅ Keyboard accessible (ESC to close)</li>
        </ul>
      </div>
    </div>
  );
};

export default NotificationDemo;

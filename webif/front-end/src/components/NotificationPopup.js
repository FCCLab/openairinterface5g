import React, { useEffect } from 'react';
import './NotificationPopup.css';

const NotificationPopup = ({ 
  message, 
  isVisible, 
  onClose, 
  onRetry, 
  type = 'error',
  autoClose = false,
  autoCloseDelay = 5000,
  position = 'bottom-right', // 'top-right', 'top-left', 'bottom-right', 'bottom-left'
  index = 0,
  totalCount = 1,
  counter = 1,
  isRemoving = false
}) => {
  useEffect(() => {
    if (autoClose && isVisible) {
      const timer = setTimeout(() => {
        onClose();
      }, autoCloseDelay);

      return () => clearTimeout(timer);
    }
  }, [isVisible, autoClose, autoCloseDelay, onClose]);



  // Handle keyboard events
  useEffect(() => {
    const handleKeyDown = (event) => {
      if (isVisible) {
        if (event.key === 'Escape') {
          onClose();
        } else if (event.key === 'Enter' && onRetry) {
          onRetry();
        }
      }
    };

    if (isVisible) {
      document.addEventListener('keydown', handleKeyDown);
    }

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
    };
  }, [isVisible, onClose, onRetry]);

  if (!isVisible) return null;

  const getIcon = () => {
    switch (type) {
      case 'error':
        return '❌';
      case 'warning':
        return '⚠️';
      case 'info':
        return 'ℹ️';
      case 'success':
        return '✅';
      default:
        return '❌';
    }
  };

  const getTitle = () => {
    let typeText;
    switch (type) {
      case 'error':
        typeText = 'Error';
        break;
      case 'warning':
        typeText = 'Warning';
        break;
      case 'info':
        typeText = 'Info';
        break;
      case 'success':
        typeText = 'Success';
        break;
      default:
        typeText = 'Error';
    }
    
    return `#${counter} ${typeText}`;
  };

  return (
    <div 
      className={`notification-popup ${isRemoving ? 'notification-removing' : ''}`} 
      onClick={(e) => e.stopPropagation()} 
      style={{ zIndex: 9999 + index }}
    >
        <div className={`notification-popup-header ${type}`}>
          <div className="notification-popup-icon">{getIcon()}</div>
          <h3 className="notification-popup-title">{getTitle()}</h3>
          <button className="notification-popup-close" onClick={onClose}>
            ×
          </button>
        </div>
        
        <div className="notification-popup-content">
          <p className="notification-popup-message">{message}</p>
        </div>
        
        <div className="notification-popup-actions">
          {onRetry && (
            <button className="notification-popup-btn notification-popup-retry" onClick={onRetry}>
              Retry
            </button>
          )}
          <button className="notification-popup-btn notification-popup-close-btn" onClick={onClose}>
            Close
          </button>
        </div>
    </div>
  );
};

export default NotificationPopup;

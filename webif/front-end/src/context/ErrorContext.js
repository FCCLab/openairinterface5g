import React, { createContext, useContext, useState, useCallback } from 'react';
import NotificationPopup from '../components/NotificationPopup';

const ErrorContext = createContext();

export const useError = () => {
  const context = useContext(ErrorContext);
  if (!context) {
    throw new Error('useError must be used within an ErrorProvider');
  }
  return context;
};

export const ErrorProvider = ({ children }) => {
  const [notifications, setNotifications] = useState([]);

  const showError = useCallback(({
    message,
    type = 'error',
    onRetry = null,
    autoClose = true,
    autoCloseDelay = 5000,
    position = 'bottom-right'
  }) => {
    const id = Date.now() + Math.random();
    const notification = {
      id,
      message,
      type,
      onRetry,
      autoClose,
      autoCloseDelay,
      position,
      isVisible: true
    };
    
    setNotifications(prev => [...prev, notification]);
    
    // Auto-remove notification after delay
    if (autoClose) {
      setTimeout(() => {
        removeNotification(id);
      }, autoCloseDelay);
    }
  }, []);

  const removeNotification = useCallback((id) => {
    setNotifications(prev => prev.filter(notification => notification.id !== id));
  }, []);

  const hideError = useCallback(() => {
    // Remove the most recent notification
    setNotifications(prev => prev.slice(0, -1));
  }, []);

  const showSuccess = useCallback((message, autoClose = true) => {
    showError({
      message,
      type: 'success',
      autoClose,
      autoCloseDelay: 3000
    });
  }, [showError]);

  const showWarning = useCallback((message, onRetry = null) => {
    showError({
      message,
      type: 'warning',
      onRetry
    });
  }, [showError]);

  const showInfo = useCallback((message, autoClose = true) => {
    showError({
      message,
      type: 'info',
      autoClose,
      autoCloseDelay: 4000
    });
  }, [showError]);

  const handleRetry = useCallback((notification) => {
    if (notification.onRetry) {
      notification.onRetry();
    }
    removeNotification(notification.id);
  }, [removeNotification]);

  const value = {
    showError,
    showSuccess,
    showWarning,
    showInfo,
    hideError
  };

  return (
    <ErrorContext.Provider value={value}>
      {children}
      <div className="notifications-container">
        {notifications.map((notification, index) => (
          <NotificationPopup
            key={notification.id}
            message={notification.message}
            isVisible={notification.isVisible}
            onClose={() => removeNotification(notification.id)}
            onRetry={notification.onRetry ? () => handleRetry(notification) : null}
            type={notification.type}
            autoClose={notification.autoClose}
            autoCloseDelay={notification.autoCloseDelay}
            position={notification.position}
            index={index}
          />
        ))}
      </div>
    </ErrorContext.Provider>
  );
};

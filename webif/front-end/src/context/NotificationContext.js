import React, { createContext, useContext, useState, useCallback } from 'react';
import NotificationPopup from '../components/NotificationPopup';

const NotificationContext = createContext();

export const useNotification = () => {
  const context = useContext(NotificationContext);
  if (!context) {
    throw new Error('useNotification must be used within a NotificationProvider');
  }
  return context;
};

export const NotificationProvider = ({ children }) => {
  const [notifications, setNotifications] = useState([]);
  const [notificationCounter, setNotificationCounter] = useState(0);

  const showError = useCallback(({
    message,
    type = 'error',
    onRetry = null,
    autoClose = true,
    autoCloseDelay = 5000,
    position = 'bottom-right'
  }) => {
    const id = Date.now() + Math.random();
    
    // Calculate the next counter value
    const nextCounter = notificationCounter + 1;
    const wrappedCounter = nextCounter > 100 ? 1 : nextCounter;
    
    // Update the counter state
    setNotificationCounter(wrappedCounter);
    
    const notification = {
      id,
      message,
      type,
      onRetry,
      autoClose,
      autoCloseDelay,
      position,
      isVisible: true,
      counter: wrappedCounter
    };
    
    setNotifications(prev => {
      const newNotifications = [notification, ...prev];
      // Keep only the latest 3 notifications, remove the oldest ones
      return newNotifications.slice(0, 3);
    });
    
    // Auto-remove notification after delay
    if (autoClose) {
      setTimeout(() => {
        removeNotification(id);
      }, autoCloseDelay);
    }
  }, [notificationCounter]);

  const removeNotification = useCallback((id) => {
    // First mark the notification as being removed for smooth animation
    setNotifications(prev => prev.map(notification => 
      notification.id === id 
        ? { ...notification, isRemoving: true }
        : notification
    ));
    
    // Then remove it after the animation completes
    setTimeout(() => {
      setNotifications(prev => prev.filter(notification => notification.id !== id));
    }, 1000); // Match the CSS transition duration
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
    <NotificationContext.Provider value={value}>
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
            totalCount={notifications.length}
            counter={notification.counter}
            isRemoving={notification.isRemoving || false}
          />
        ))}
      </div>
    </NotificationContext.Provider>
  );
};

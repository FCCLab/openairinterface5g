import React, { useState, useEffect } from 'react';
import SpectrogramContainerTemplate from '../components/SpectrogramContainer';
import UserGuideContainerTemplate from '../components/UserGuideContainer';

function SpectrogramPage() {
  const [showUserGuide, setShowUserGuide] = useState(false);

  const handleShowUserGuide = () => {
    setShowUserGuide(true);
  };

  const handleHideUserGuide = () => {
    setShowUserGuide(false);
  };

  // Update global state for button text
  useEffect(() => {
    window.showUserGuideState = showUserGuide;
    // Dispatch event to notify SpectrogramContainer of state change
    window.dispatchEvent(new CustomEvent('userGuideStateChanged'));
  }, [showUserGuide]);

  // Listen for custom event from SpectrogramContainer
  useEffect(() => {
    const handleToggleUserGuideEvent = (event) => {
      const currentState = event.detail?.currentState || false;
      setShowUserGuide(!currentState);
    };

    window.addEventListener('toggleUserGuide', handleToggleUserGuideEvent);
    
    return () => {
      window.removeEventListener('toggleUserGuide', handleToggleUserGuideEvent);
    };
  }, []);

  return (
    <div className="page-container spectrogram-page">
      <div style={{ display: 'flex', flexDirection: 'column', gap: '20px' }}>
        <SpectrogramContainerTemplate />

        {/* User Guide Section */}
        {showUserGuide && (
          <div style={{ marginTop: '20px' }}>
            <UserGuideContainerTemplate />
          </div>
        )}
      </div>
    </div>
  );
}

export default SpectrogramPage;

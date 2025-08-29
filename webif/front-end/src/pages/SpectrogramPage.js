import React, { useState } from 'react';
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

  return (
    <div className="page-container spectrogram-page">
      <div style={{ display: 'flex', flexDirection: 'column', gap: '20px' }}>
        <SpectrogramContainerTemplate />

        {!showUserGuide && (
          <button
            className="control-btn"
            style={{ alignSelf: 'flex-start', marginBottom: '1rem', width: 'fit-content' }}
            onClick={handleShowUserGuide}
          >
            Show User Guide
          </button>
        )}

        {showUserGuide && (
          <>
            <button
              className="control-btn"
              style={{ alignSelf: 'flex-start', marginBottom: '1rem', width: 'fit-content' }}
              onClick={handleHideUserGuide}
            >
              Hide User Guide
            </button>
            <UserGuideContainerTemplate />
          </>
        )}
      </div>
    </div>
  );
}

export default SpectrogramPage;

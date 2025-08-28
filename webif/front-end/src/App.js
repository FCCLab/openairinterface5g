import React, { useState, useEffect } from 'react';
import { BrowserRouter as Router, Routes, Route, Link, useLocation } from 'react-router-dom';
import './App.css';
import HomePage from './pages/HomePage';
import CellScanPage from './pages/CellScanPage';
import CellAttachedPage from './pages/CellAttachedPage';
import LogsPage from './pages/LogsPage';
import SpectrogramPage from './pages/SpectrogramPage';

function Navigation() {
  const location = useLocation();
  
  return (
    <nav className="navigation">
      <div className="nav-container">
        <div className="nav-title">
          <h1>OpenAirInterface5G</h1>
          <p>5G Network Management Interface</p>
        </div>
        <div className="nav-links">
          <Link to="/" className={`nav-link ${location.pathname === '/' ? 'active' : ''}`}>
            Home
          </Link>
          <Link to="/cell-scan" className={`nav-link ${location.pathname === '/cell-scan' ? 'active' : ''}`}>
            Cell Scan
          </Link>
          <Link to="/cell-attached" className={`nav-link ${location.pathname === '/cell-attached' ? 'active' : ''}`}>
            Cell Attached
          </Link>
                        <Link to="/logs" className={`nav-link ${location.pathname === '/logs' ? 'active' : ''}`}>
                Logs
              </Link>
              <Link to="/spectrogram" className={`nav-link ${location.pathname === '/spectrogram' ? 'active' : ''}`}>
                Spectrogram
              </Link>
        </div>
      </div>
    </nav>
  );
}

function App() {
  return (
    <Router>
      <div className="App">
        <Navigation />
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/cell-scan" element={<CellScanPage />} />
          <Route path="/cell-attached" element={<CellAttachedPage />} />
                         <Route path="/logs" element={<LogsPage />} />
               <Route path="/spectrogram" element={<SpectrogramPage />} />
        </Routes>
      </div>
    </Router>
  );
}

export default App;

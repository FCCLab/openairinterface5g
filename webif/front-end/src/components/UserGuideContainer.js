import React, { useState, useEffect } from 'react';
import ReactMarkdown from 'react-markdown';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { prism } from 'react-syntax-highlighter/dist/esm/styles/prism';
import { BlockMath } from 'react-katex';
import 'katex/dist/katex.min.css';

// Template for the user guide container
const UserGuideContainerTemplate = () => {
  // State for STFT guide content
  const [stftGuideContent, setStftGuideContent] = useState('');
  
  // Load STFT guide content from markdown file
  useEffect(() => {
    fetch('/stft.md')
      .then(response => response.text())
      .then(content => setStftGuideContent(content))
      .catch(error => {
        console.error('Error loading STFT guide:', error);
        // Fallback content if file can't be loaded
        setStftGuideContent('# STFT Parameters Guide\n\n## Mathematical Foundation\n\n### STFT Formula\n\nThe Short-Time Fourier Transform (STFT) is defined as:\n\n```\nX(τ, ω) = ∫ x(t) · w(t-τ) · e^(-jωt) dt\n```\n\n**Where:**\n- **X(τ, ω)**: STFT output (time-frequency representation)\n- **x(t)**: Input signal\n- **w(t-τ)**: Window function centered at time τ\n- **e^(-jωt)**: Complex exponential (Fourier kernel)');
      });
  }, []);

  // Smooth scroll to section function
  const scrollToSection = (sectionId) => {
    console.log('Scrolling to section:', sectionId);
    const element = document.getElementById(sectionId);
    console.log('Found element:', element);
    
    if (element) {
      const offset = 100; // Offset to account for fixed header
      const elementPosition = element.getBoundingClientRect().top;
      const offsetPosition = elementPosition + window.pageYOffset - offset;
      
      console.log('Scrolling to position:', offsetPosition);
      
      window.scrollTo({
        top: offsetPosition,
        behavior: 'smooth'
      });
    } else {
      console.log('Element not found, searching for elements with IDs:');
      const allElements = document.querySelectorAll('[id]');
      allElements.forEach(el => console.log('Element with ID:', el.id, el));
    }
  };

  // Template for navigation items
  const navigationItemsTemplate = [
    { id: 'fft', label: 'Fast Fourier Transform' },
    { id: 'stft-formula', label: 'STFT Formula' },
    { id: 'window-functions', label: 'Window Functions' },
    { id: 'core-parameters', label: 'Core Parameters' },
    { id: 'stft-parameters', label: 'STFT Parameters' },
    { id: 'display-parameters', label: 'Display Parameters' },
    { id: 'best-practices', label: 'Best Practices' }
  ];

  // Template for navigation button styles
  const navigationButtonStyle = {
    color: '#666',
    textDecoration: 'none',
    display: 'block',
    padding: '0.3rem 0',
    background: 'none',
    border: 'none',
    cursor: 'pointer',
    textAlign: 'left',
    width: '100%'
  };

  // Template for navigator styles
  const navigatorStyle = {
    position: 'sticky',
    top: '20px',
    right: '20px',
    background: 'white',
    border: '1px solid #e0e0e0',
    borderRadius: '8px',
    padding: '1rem',
    boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
    maxWidth: '250px',
    maxHeight: '80vh',
    overflowY: 'auto',
    zIndex: 1000,
    float: 'right',
    marginLeft: '20px',
    marginBottom: '20px'
  };

  // Template for container styles
  const containerStyle = {
    minHeight: '40vh',
    overflowY: 'auto',
    padding: '20px',
    border: '1px solid #e0e0e0',
    borderRadius: '8px',
    backgroundColor: 'white'
  };

  // Template for guide section styles
  const guideSectionStyle = {
    padding: '2rem',
    backgroundColor: 'white',
    borderRadius: '12px',
    border: '1px solid rgba(139, 92, 246, 0.3)',
    boxShadow: '0 4px 20px rgba(0, 0, 0, 0.1)',
    position: 'relative'
  };

  // Template for ReactMarkdown components
  const markdownComponentsTemplate = {
    h1: ({children}) => (
      <h1 style={{
        color: '#8b5cf6',
        fontSize: '2rem',
        textAlign: 'left',
        margin: '2rem 0 1.5rem 0'
      }}>
        {children}
      </h1>
    ),
    h2: ({children}) => {
      const text = String(children);
      let id = '';
      if (text.includes('Core Parameters')) id = 'core-parameters';
      else if (text.includes('STFT-Specific Parameters')) id = 'stft-parameters';
      else if (text.includes('Display Parameters')) id = 'display-parameters';
      else if (text.includes('Best Practices')) id = 'best-practices';
      
      return (
        <h2 id={id} style={{
          color: '#8b5cf6',
          fontSize: '1.6rem',
          borderBottom: '1px solid rgba(139, 92, 246, 0.3)',
          paddingBottom: '0.5rem',
          margin: '2rem 0 1rem 0',
          textAlign: 'left'
        }}>
          {children}
        </h2>
      );
    },
    h3: ({children}) => {
      const text = String(children);
      let id = '';
      if (text.includes('Fast Fourier Transform')) id = 'fft';
      else if (text.includes('Discrete STFT Formula')) id = 'stft-formula';
      else if (text.includes('STFT Window Functions')) id = 'window-functions';
      
      return (
        <h3 id={id} style={{
          color: '#8b5cf6',
          fontSize: '1.3rem',
          margin: '1.5rem 0 1rem 0',
          textAlign: 'left'
        }}>
          {children}
        </h3>
      );
    },
    p: ({children}) => (
      <p style={{
        color: '#333',
        margin: '1rem 0',
        textAlign: 'left'
      }}>
        {children}
      </p>
    ),
    strong: ({children}) => (
      <strong style={{color: '#8b5cf6'}}>
        {children}
      </strong>
    ),
    code: ({node, inline, className, children, ...props}) => {
      const match = /language-(\w+)/.exec(className || '');
      if (!inline && match) {
        if (match[1] === 'math') {
          return (
            <div style={{
              textAlign: 'center',
              margin: '1.5rem 0',
              padding: '1rem',
              backgroundColor: 'rgba(139, 92, 246, 0.1)',
              borderRadius: '8px'
            }}>
              <BlockMath math={String(children).replace(/\n$/, '')} />
            </div>
          );
        }
        return (
          <div style={{margin: '1.5rem 0'}}>
            <SyntaxHighlighter
              language={match[1]}
              PreTag="div"
              {...props}
              style={{
                ...prism,
                background: '#f8f9fa',
                borderRadius: '8px',
                padding: '1rem',
                fontSize: '14px',
                fontFamily: 'Consolas, Monaco, "Andale Mono", "Ubuntu Mono", monospace'
              }}
            >
              {String(children).replace(/\n$/, '')}
            </SyntaxHighlighter>
          </div>
        );
      }
      // Inline code
      return (
        <code style={{
          background: 'rgba(139, 92, 246, 0.1)',
          padding: '2px 4px',
          borderRadius: '3px',
          fontFamily: 'monospace',
          color: '#8b5cf6',
          fontSize: '0.9em',
          border: '1px solid rgba(139, 92, 246, 0.2)',
          textAlign: 'left'
        }}>
          {children}
        </code>
      );
    },
    ul: ({children}) => (
      <ul style={{
        color: '#333',
        margin: '1rem 0',
        paddingLeft: '1.5rem',
        textAlign: 'left'
      }}>
        {children}
      </ul>
    ),
    ol: ({children}) => (
      <ol style={{
        color: '#333',
        margin: '1rem 0',
        paddingLeft: '1.5rem',
        textAlign: 'left'
      }}>
        {children}
      </ol>
    ),
    li: ({children}) => (
      <li style={{
        color: '#333',
        margin: '0.5rem 0',
        textAlign: 'left'
      }}>
        {children}
      </li>
    ),
    img: ({src, alt, ...props}) => (
      <div style={{textAlign: 'center', margin: '1.5rem 0'}}>
        <img 
          src={src} 
          alt={alt} 
          style={{
            maxWidth: '100%',
            height: 'auto',
            borderRadius: '8px',
            boxShadow: '0 4px 8px rgba(0,0,0,0.1)'
          }}
          {...props}
        />
      </div>
    )
  };

  return (
    <div className="user-guide-container" style={containerStyle}>
      {/* STFT Parameters Guide */}
      <div className="stft-guide-section" style={guideSectionStyle}>
        {/* Guide Navigator */}
        <div className="guide-navigator" style={navigatorStyle}>
          <h4 style={{
            margin: '0 0 1rem 0',
            color: '#8b5cf6',
            fontSize: '1.1rem'
          }}>
            Guide Navigation
          </h4>
          <div style={{ fontSize: '0.9rem' }}>
            {navigationItemsTemplate.map((item, index) => (
              <div key={index} style={{ marginBottom: '0.5rem' }}>
                <button 
                  onClick={() => scrollToSection(item.id)}
                  style={navigationButtonStyle}
                >
                  {item.label}
                </button>
              </div>
            ))}
          </div>
        </div>

        <div style={{ overflow: 'hidden' }}>
          <ReactMarkdown components={markdownComponentsTemplate}>
            {stftGuideContent}
          </ReactMarkdown>
        </div>
      </div>
    </div>
  );
};

export default UserGuideContainerTemplate;

/**
 * Singleton UE Manager
 * Persistent UE manager that exists independently of frontend connections
 */

const UeManagerReal = require('./UeManagerReal');
const UeManagerSimulated = require('./UeManagerSimulated');

class SingletonUeManager {
  constructor() {
    if (SingletonUeManager.instance) {
      return SingletonUeManager.instance;
    }

    // Initialize state
    this.currentMode = 'real';
    this.simulatedManager = null;
    this.realManager = null;
    this.isInitialized = false;
    
    // Make this instance persistent
    SingletonUeManager.instance = this;
    
    // Initialize the default manager
    this.initialize();
  }

  /**
   * Initialize the singleton manager
   */
  initialize() {
    if (this.isInitialized) return;
    
    console.log('Initializing Singleton UE Manager...');
    
    try {
      // Create simulated manager instance
      this.simulatedManager = new UeManagerSimulated();
      console.log('Simulated UE Manager created successfully');
      
      // Create real manager instance (if needed)
      this.realManager = UeManagerReal; // UeManagerReal is already an instance
      console.log('Real UE Manager created successfully');
      
      this.isInitialized = true;
      console.log('Singleton UE Manager initialized successfully');
    } catch (error) {
      console.error('Error initializing Singleton UE Manager:', error);
      throw error;
    }
  }

  /**
   * Set the current UE mode
   * @param {string} mode - 'simulated' or 'real'
   */
  setMode(mode) {
    if (mode !== 'simulated' && mode !== 'real') {
      throw new Error('Invalid UE mode. Must be "simulated" or "real"');
    }
    
    if (this.currentMode !== mode) {
      console.log(`Switching UE mode from ${this.currentMode} to ${mode}`);
      this.currentMode = mode;
    }
  }

  /**
   * Get the current UE mode
   * @returns {string} Current mode
   */
  getMode() {
    return this.currentMode;
  }

  /**
   * Get the current UE manager instance
   * @returns {Object} Current UE manager
   */
  getCurrentManager() {
    if (!this.isInitialized) {
      this.initialize();
    }

    switch (this.currentMode) {
      case 'simulated':
        return this.simulatedManager;
      case 'real':
        return this.realManager;
      default:
        throw new Error(`Unknown UE mode: ${this.currentMode}`);
    }
  }

  /**
   * Get status of the singleton manager
   * @returns {Object} Manager status
   */
  getManagerStatus() {
    return {
      isInitialized: this.isInitialized,
      currentMode: this.currentMode,
      simulatedManagerExists: !!this.simulatedManager,
      realManagerExists: !!this.realManager,
      timestamp: new Date().toISOString()
    };
  }

  /**
   * Force reinitialize the manager (useful for debugging)
   */
  reinitialize() {
    console.log('Reinitializing Singleton UE Manager...');
    this.isInitialized = false;
    this.simulatedManager = null;
    this.realManager = null;
    this.initialize();
  }
}

// Create the singleton instance
const singletonUeManager = new SingletonUeManager();

// Export the singleton instance methods
module.exports = {
  setMode: (mode) => singletonUeManager.setMode(mode),
  getMode: () => singletonUeManager.getMode(),
  getCurrentManager: () => singletonUeManager.getCurrentManager(),
  getManagerStatus: () => singletonUeManager.getManagerStatus(),
  reinitialize: () => singletonUeManager.reinitialize()
};

/**
 * Unified UE Interface
 * This interface defines the common methods that both simulated and real UE implementations must provide.
 * This allows the system to switch between simulated and real UE without changing the frontend or API.
 */

class UeInterface {
  /**
   * Get current UE status
   * @returns {Object} Status object with isRunning, processId, connectionState, uptime, status
   */
  getStatus() {
    throw new Error('getStatus() must be implemented by UE implementation');
  }

  /**
   * Get UE configuration
   * @returns {Object} Configuration object
   */
  getConfig() {
    throw new Error('getConfig() must be implemented by UE implementation');
  }

  /**
   * Update UE configuration
   * @param {Object} newConfig - New configuration to apply
   * @returns {Object} Updated configuration
   */
  updateConfig(newConfig) {
    throw new Error('updateConfig() must be implemented by UE implementation');
  }

  /**
   * Start UE process
   * @param {Object} config - Configuration for starting the UE
   * @returns {Promise<Object>} Result object with success, message, processId, etc.
   */
  async start(config = {}) {
    throw new Error('start() must be implemented by UE implementation');
  }

  /**
   * Stop UE process
   * @returns {Promise<Object>} Result object with success, message
   */
  async stop() {
    throw new Error('stop() must be implemented by UE implementation');
  }

  /**
   * Restart UE process
   * @param {Object} config - Configuration for restarting the UE
   * @returns {Promise<Object>} Result object with success, message, processId, etc.
   */
  async restart(config = {}) {
    throw new Error('restart() must be implemented by UE implementation');
  }

  /**
   * Check if UE process is running
   * @returns {boolean} True if running, false otherwise
   */
  isRunning() {
    throw new Error('isRunning() must be implemented by UE implementation');
  }

  /**
   * Get UE logs
   * @param {number} lines - Number of log lines to return
   * @returns {Array} Array of log entries
   */
  getLogs(lines = 100) {
    throw new Error('getLogs() must be implemented by UE implementation');
  }

  /**
   * Get UE performance metrics
   * @returns {Object} Metrics object
   */
  getMetrics() {
    throw new Error('getMetrics() must be implemented by UE implementation');
  }

  /**
   * Start cell scanning
   * @param {Object} scanConfig - Scan configuration (frequency, etc.)
   * @returns {Promise<Object>} Result object with success, message, scanId
   */
  async startScan(scanConfig) {
    throw new Error('startScan() must be implemented by UE implementation');
  }

  /**
   * Stop cell scanning
   * @returns {Promise<Object>} Result object with success, message
   */
  async stopScan() {
    throw new Error('stopScan() must be implemented by UE implementation');
  }

  /**
   * Get detected cells
   * @returns {Promise<Object>} Result object with success, cells array, count, timestamp
   */
  async getDetectedCells() {
    throw new Error('getDetectedCells() must be implemented by UE implementation');
  }

  /**
   * Get scan status
   * @returns {Object} Scan status object
   */
  getScanStatus() {
    throw new Error('getScanStatus() must be implemented by UE implementation');
  }

  /**
   * Send command to UE process (if supported)
   * @param {string} command - Command to send
   * @returns {Promise<Object>} Result object
   */
  async sendCommand(command) {
    throw new Error('sendCommand() must be implemented by UE implementation');
  }
}

module.exports = UeInterface;

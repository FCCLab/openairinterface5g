const express = require('express');
const logger = require('../logger');
const usrpDeviceManager = require('../UsrpDeviceManager');

const router = express.Router();

// Get USRP devices
router.get('/devices', async (req, res) => {
  try {
    logger.api('USRP devices requested', { ip: req.ip });
    const usrpInfo = await usrpDeviceManager.getUsrpDevices();
    res.json(usrpInfo);
  } catch (error) {
    logger.error('Error in USRP devices API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get USRP devices' });
  }
});

// Refresh USRP cache
router.post('/refresh', async (req, res) => {
  try {
    logger.api('USRP cache refresh requested', { ip: req.ip });
    const usrpInfo = await usrpDeviceManager.forceUpdate();
    res.json(usrpInfo);
  } catch (error) {
    logger.error('Error in USRP refresh API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to refresh USRP devices' });
  }
});

// Get USRP cache stats
router.get('/cache/stats', (req, res) => {
  try {
    logger.api('USRP cache stats requested', { ip: req.ip });
    const stats = usrpDeviceManager.getCacheStats();
    res.json(stats);
  } catch (error) {
    logger.error('Error in USRP cache stats API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get USRP cache stats' });
  }
});

// Get USRP device details
router.get('/device/details', async (req, res) => {
  try {
    const { deviceArgs } = req.query;
    if (!deviceArgs) {
      return res.status(400).json({ error: 'deviceArgs parameter is required' });
    }

    logger.api('USRP device details requested', { ip: req.ip, deviceArgs });
    const details = await usrpDeviceManager.getDeviceDetails(deviceArgs);
    res.json(details);
  } catch (error) {
    logger.error('Error in USRP device details API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get USRP device details' });
  }
});

module.exports = router;

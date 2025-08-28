# OpenAirInterface5G Web Interface

A complete web interface for OpenAirInterface5G with separate frontend and backend servers.

## 🏗️ Architecture

- **Frontend**: React.js application running on port 41000
- **Backend**: Express.js API server running on port 40000
- **Communication**: Frontend proxies API calls to backend

## 🚀 Quick Start

### Option 1: Automatic Startup (Recommended)
```bash
cd webif
./start-servers.sh
```

This script will:
- Check port availability
- Install dependencies for both servers
- Start backend server on port 40000
- Start frontend server on port 41000
- Provide URLs for access

### Option 2: Manual Startup

#### Start Backend Server
```bash
cd webif/back-end
npm install
npm start
```
Backend will be available at: http://localhost:40000

#### Start Frontend Server (in a new terminal)
```bash
cd webif/front-end
npm install
npm start
```
Frontend will be available at: http://localhost:41000

## 📊 Access URLs

- **Frontend UI**: http://localhost:41000
- **Backend API**: http://localhost:40000
- **API Documentation**: http://localhost:40000/api
- **Health Check**: http://localhost:40000/health

## 🔧 Configuration

### Port Configuration
- Frontend: Port 41000 (configurable via PORT environment variable)
- Backend: Port 40000 (configurable via PORT environment variable)

### Environment Variables
```bash
# Backend
export PORT=40000
export NODE_ENV=development

# Frontend
export PORT=41000
```

## 📁 Project Structure

```
webif/
├── back-end/           # Express.js backend server
│   ├── app.js         # Main server file
│   ├── package.json   # Backend dependencies
│   └── README.md      # Backend documentation
├── front-end/         # React.js frontend application
│   ├── src/           # React source code
│   ├── public/        # Static files
│   ├── package.json   # Frontend dependencies
│   └── README.md      # Frontend documentation
├── start-servers.sh   # Startup script
└── README.md         # This file
```

## 🛠️ Development

### Backend Development
```bash
cd webif/back-end
npm run dev  # Uses nodemon for auto-restart
```

### Frontend Development
```bash
cd webif/front-end
npm start    # React development server with hot reload
```

## 🔍 Troubleshooting

### Port Already in Use
If you get "port already in use" errors:

1. **Check what's using the port:**
   ```bash
   lsof -i :41000  # Check frontend port
   lsof -i :40000  # Check backend port
   ```

2. **Kill the process:**
   ```bash
   kill -9 <PID>  # Replace <PID> with the process ID
   ```

### Frontend Won't Start
1. Check if backend is running on port 40000
2. Verify all dependencies are installed: `npm install`
3. Check for syntax errors in React components
4. Clear browser cache and try again

### Backend Won't Start
1. Check if port 40000 is available
2. Verify Node.js version (requires Node.js 14+)
3. Check for missing system dependencies
4. Review error logs in terminal

### API Connection Issues
1. Ensure backend is running on port 40000
2. Check CORS configuration in backend
3. Verify proxy settings in frontend package.json
4. Check browser console for network errors

## 🚀 Production Deployment

### Build Frontend
```bash
cd webif/front-end
npm run build
```

### Deploy Backend
```bash
cd webif/back-end
export NODE_ENV=production
npm start
```

### Using PM2 (Recommended)
```bash
# Install PM2 globally
npm install -g pm2

# Start backend
cd webif/back-end
pm2 start app.js --name "oai5g-backend"

# Start frontend (if needed)
cd webif/front-end
pm2 start npm --name "oai5g-frontend" -- start

# Save PM2 configuration
pm2 save
pm2 startup
```

## 📚 API Documentation

The backend provides comprehensive system monitoring APIs:

- `GET /api/system/cpu` - CPU information and temperature
- `GET /api/system/memory` - Memory usage information
- `GET /api/system/disk` - Disk usage information
- `GET /api/system/network` - Network interfaces
- `GET /api/system/uptime` - System uptime
- `GET /api/system/info` - Complete system information

Visit http://localhost:40000/api for full API documentation.

## Logging System

The backend includes a comprehensive logging system with the following features:

### **Log Files**
- **`logs/combined-YYYY-MM-DD.log`** - All log levels (30 days retention)
- **`logs/error-YYYY-MM-DD.log`** - Error logs only (14 days retention)
- **`logs/access-YYYY-MM-DD.log`** - HTTP access logs (30 days retention)
- **`logs/exceptions-YYYY-MM-DD.log`** - Uncaught exceptions (14 days retention)
- **`logs/rejections-YYYY-MM-DD.log`** - Unhandled promise rejections (14 days retention)

### **Log Levels**
- **`error`** - Errors and exceptions
- **`warn`** - Security warnings and important events
- **`info`** - General information and API requests
- **`debug`** - Detailed debugging information

### **Log Types**
- **`api`** - API requests and responses
- **`system`** - System events (startup, shutdown)
- **`security`** - Security-related events
- **`performance`** - Performance metrics
- **`http`** - HTTP access logs

### **Viewing Logs**

#### **Using npm scripts:**
```bash
cd webif/back-end
npm run logs:list                    # List available log files
npm run logs:view combined-2024-01-15.log  # View specific log file
npm run logs:tail combined-2024-01-15.log  # Follow log in real-time
```

#### **Using the log viewer directly:**
```bash
cd webif/back-end
node view-logs.js list                    # List available log files
node view-logs.js view combined-2024-01-15.log  # View last 50 lines
node view-logs.js view error-2024-01-15.log 100 # View last 100 lines
node view-logs.js tail combined-2024-01-15.log  # Follow log in real-time
```

#### **Using standard tools:**
```bash
cd webif/back-end/logs
tail -f combined-2024-01-15.log           # Follow log in real-time
grep "ERROR" combined-2024-01-15.log      # Search for errors
jq '.' combined-2024-01-15.log | head -20 # Pretty print JSON logs
```

### **Log Configuration**
- **Log Level**: Set via `LOG_LEVEL` environment variable (default: `info`)
- **Retention**: Automatic daily rotation with compression
- **Size Limits**: 20MB per file before rotation
- **Format**: JSON format for easy parsing and analysis

## 🔒 Security Features

- Helmet.js security headers
- CORS protection
- Rate limiting (100 requests per 15 minutes)
- Input validation and sanitization
- Secure error handling

## 📈 Performance Features

- Gzip compression
- HTTP caching headers
- Optimized API responses
- Graceful shutdown handling

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test both frontend and backend
5. Submit a pull request

## 📄 License

ISC License - see individual component README files for details.

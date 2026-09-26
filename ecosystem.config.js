const fs = require('fs');
const path = require('path');

function resolveServerBinary() {
  const candidates = [
    path.join(__dirname, 'build', 'TinyTroopersServer'),
    path.join(__dirname, 'build', 'TinyTroopersServer.exe'),
    path.join(__dirname, 'build', 'Release', 'TinyTroopersServer'),
    path.join(__dirname, 'build', 'Release', 'TinyTroopersServer.exe')
  ];

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  return process.platform === 'win32'
    ? path.join(__dirname, 'build', 'Release', 'TinyTroopersServer.exe')
    : path.join(__dirname, 'build', 'TinyTroopersServer');
}

module.exports = {
  apps: [
    {
      name: 'tinytroopers-server',
      script: resolveServerBinary(),
      cwd: __dirname,
      autorestart: true,
      max_restarts: 10,
      restart_delay: 2000,
      watch: false,
      instances: 1,
      exec_mode: 'fork',
      env: {
        NODE_ENV: 'production'
      }
    }
  ]
};

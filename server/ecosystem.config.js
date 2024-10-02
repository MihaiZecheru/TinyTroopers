module.exports = {
  apps: [{
    name: "TinyTroopersServer",
    script: "/home/TinyTroopersServer/src/index.ts",
    watch: true,
    ignore_watch: ["node_modules"],
    interpreter: "ts-node",
  }]
}

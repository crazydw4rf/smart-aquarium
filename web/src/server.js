Bun.serve({
  port: 3000,
  fetch(req, server) {
    const url = new URL(req.url);

    if (url.pathname == "/") {
      return new Response(Bun.file(__dirname + "/index.html"));
    } else if (url.pathname == "/aqua") {
      if (server.upgrade(req)) return;
      return new Response("Upgrade failed!", { status: 400 });
    } else if (/\/[a-zA-Z].+/.test(url.pathname)) {
      try {
        // FIXME: coba cara lain kalau kaya gini jadi gak ke cache
        return new Response(Bun.file(`${__dirname + url.pathname}`));
      } catch (error) {
        return new Response("Not Found", { status: 404 });
      }
    }

    return new Response("Not Found", { status: 404 });
  },
  websocket: {
    open: () => console.log("Tersambung boss"),
    close: () => console.log("Good bye boss c u later"),
    async message(ws, message) {
      const payload = JSON.parse(message);
      const actionType = payload.action;

      if (actionType == "ping") {
        ws.send(
          JSON.stringify({
            monitor: "water_level",
            data: 98,
          }),
        );
        ws.send(
          JSON.stringify({
            monitor: "water_temp",
            data: 30,
          }),
        );
      }
    },
  },
});

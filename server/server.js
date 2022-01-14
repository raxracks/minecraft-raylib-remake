const WebSocketServer = require('websocket').server;
const http = require('http');
const player = require('./player.js');
const actionSystem = require('./action-system.js');
const Config = require('./config.js');
const PlayerManager = new player.PlayerManager(Config);
const ActionSystem = new actionSystem.ActionSystem(PlayerManager, Config);
const port = 8080;

const server = http.createServer(function (request, response) {
	console.log(`[${new Date()}] Received request for ${request.url}`);
	response.writeHead(404); // could send them to a page instead
	response.end();
});

server.listen(port, function () {
	console.log(`[${new Date()}] Server is listening on port ${port}`);
});

wsServer = new WebSocketServer({
	httpServer: server,
	autoAcceptConnections: false
});

function BroadcastUpdate(id, action, data) {
	let players = PlayerManager.GetPlayers();

	Object.keys(players).forEach(playerID => {
		if(playerID != id) players[playerID].connection.sendUTF(`${new Date()} | ${id} | ${action} | ${JSON.stringify(data)}`);
	});
}

wsServer.on('request', function (request) {
	// if (!originIsAllowed(request.origin)) {
	// 	request.reject();
	// 	console.log((new Date()) + ' Connection from origin ' + request.origin + ' rejected.');
	// 	return;
	// }

	var connection = request.accept('echo-protocol', request.origin);

	console.log(`[${new Date()}] Connection accepted.`);

	PlayerManager.AddPlayer(request.key, connection);

	connection.on('message', function (message) {
		if (message.type === 'utf8') {
			if(message.utf8Data.length > 0) {
				let split = message.utf8Data.split(" | ");
				let action = split[0];
				let data = JSON.parse(split[1]);

				console.log(message.utf8Data);

				if(ActionSystem.RequestAction(request.key, action, data) == 1) {
					BroadcastUpdate(request.key, action, data);
				};
			};

			connection.sendBytes(Buffer.from([0]));
		}
		else if (message.type === 'binary') {
			connection.sendBytes(message.binaryData);
		}
	});
	connection.on('close', function (reasonCode, description) {
		console.log(`[${new Date()}] Peer ${connection.remoteAddress} disconnected.`);

		PlayerManager.RemovePlayer(request.key);

		BroadcastUpdate(request.key, "RemovePlayer", {});
	});
});

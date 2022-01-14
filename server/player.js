module.exports.PlayerManager = class {
    constructor(Config) {
        this.players = {};

        this.AddPlayer = function(id, connection, type) {
            this.players[id] = {connection};

            console.log(`[${new Date()}] [PlayerManager] Player ${id} connected.`);

            this.PopulateData(id);
        }

        this.RemovePlayer = function(id) {
            delete this.players[id];

            console.log(`[${new Date()}] [PlayerManager] Player ${id} disconnected.`);
        }

        this.PopulateData = function(id) {
            Config.PopulateData(id, this);
        }

        this.GetPlayer = function(id) {
            return this.players[id];
        }

        this.UpdatePlayer = function(id, data) {
            this.players[id] = data;
        }

        this.GetPlayers = function(id) {
            return this.players;
        }
    }
}
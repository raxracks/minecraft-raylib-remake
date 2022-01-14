module.exports.ActionSystem = class {
    constructor(PlayerManager, Config) {
        this.RequestAction = function(id, action, data) {
            return Config.Actions[action](id, this, PlayerManager, data);
        }

        this.SendLocalAction = function(connection, id, action, data) {
            connection.sendUTF(`${new Date()} | ${id} | ${action} | ${JSON.stringify(data)}`);
        }
    }
}
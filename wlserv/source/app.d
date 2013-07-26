module wlserv.app;

import vibe.d;
import vibe.data.json;

import wlserv.game;
import wlserv.user;

struct Args
{
    HTTPServerRequest req;
    alias req this;

    this(HTTPServerRequest req)
    {
        this.req = req;
    }

    string opIndex(string key)
    {
        if(key in req.params)
            return req.params[key];
        else if(key in req.form)
            return req.form[key];
        else if(key in req.query)
            return req.query[key];
        else
            return null;
    }

    string opDispatch(string Key)()
    {
        return this[Key];
    }
}

Args args(HTTPServerRequest req)
{
    return Args(req);
}

// http handlers
void error(HTTPServerResponse res, string type = null)
{
    Json j = Json.EmptyObject;
    j.status = "error";
    if(type)
        j.error = type;
    res.writeBody(j.toPrettyString(), "text/plain");
}

void errorPage(HTTPServerRequest req, HTTPServerResponse res, HTTPServerErrorInfo error)
{
	.error(res);
}

void cron()
{
    // logout inactive users
}

void createAccount(HTTPServerRequest req, HTTPServerResponse res)
{
    string user = req.args.user;
    string pass = req.args.pass;
    if(!user || !pass)
        return error(res, "missing arguments");

    User u = createUser(user, pass);
    if(!u)
        return error(res, "already exists");

    u.newSession();

    Json j = Json.EmptyObject;
    j.status = "success";
    j.user = u.toJson(true);

	res.writeBody(j.toPrettyString(), "text/plain");
}

void getUser(HTTPServerRequest req, HTTPServerResponse res)
{
    string user = req.args.user;
    if(!user)
        return error(res, "missing arguments");

    User u = getUserByName(user);
    if(!u)
        return error(res);

    Json j = Json.EmptyObject;
    j.status = "success";
    j.user = u.toJson(false);

	res.writeBody(j.toPrettyString(), "text/plain");
}

void getMap(HTTPServerRequest req, HTTPServerResponse res)
{
    string map = req.args.map;
    if(!map)
        return error(res, "missing arguments");

	res.writeBody("map...", "text/plain");
}

void login(HTTPServerRequest req, HTTPServerResponse res)
{
    string user = req.args.user;
    string pass = req.args.pass;
    if(!user || !pass)
        return error(res, "missing arguments");

    User u = wlserv.user.getUser(user, pass);
    if(!u)
        return error(res);

    u.lastSeen = Clock.currTime();
    u.newSession();

    Json j = Json.EmptyObject;
    j.status = "success";
    j.user = u.toJson(true);

	res.writeBody(j.toPrettyString(), "text/plain");
}

User loggedInUser(HTTPServerRequest req)
{
    User u = getUserByName(req.args.user);
    if(!u || icmp(u.sessionKey, req.args.key) != 0)
        return null;
    return u;
}

void checkLogin(HTTPServerRequest req, HTTPServerResponse res)
{
    if(!req.args.user || !req.args.key)
        return error(res, "missing arguments");

    User u = loggedInUser(req);
    if(!u)
        return error(res, "not logged in");

    u.lastSeen = Clock.currTime();
}

void logout(HTTPServerRequest req, HTTPServerResponse res)
{
    User u = loggedInUser(req);
    u.sessionKey = null;

    Json j = Json.EmptyObject;
    j.status = "success";

	res.writeBody(j.toPrettyString(), "text/plain");
}

void createLobby(HTTPServerRequest req, HTTPServerResponse res)
{
    string map = req.args.map;
    string numplayers = req.args.numplayers;
    if(!map || !numplayers)
        return error(res, "missing arguments");

    User u = loggedInUser(req);

    Lobby l = Lobby.create(map, to!int(numplayers), u);

    Json j = Json.EmptyObject;
    j.status = "success";
    j.lobby = "lobby..."; // l.toJson();

	res.writeBody(j.toPrettyString(), "text/plain");
}

void getGame(HTTPServerRequest req, HTTPServerResponse res)
{
    string game = req.args.game;
    if(!game)
        return error(res, "missing arguments");

    Json j = Json.EmptyObject;
    j.status = "success";
    j.game = "game..."; // g.toJson();
    
	res.writeBody(j.toPrettyString(), "text/plain");
}

shared static this()
{
    Timer timer = setTimer(dur!"minutes"(1), toDelegate(&cron), true);

	auto settings = new HTTPServerSettings;
    settings.errorPageHandler = toDelegate(&errorPage);
	settings.port = 8888;
    settings.options = HTTPServerOption.parseURL | HTTPServerOption.parseFormBody | HTTPServerOption.parseQueryString;

    auto router = new URLRouter;
    router.any("/api/createaccount", &createAccount);
    router.any("/api/users/:user", &getUser);
    router.any("/api/maps/:map", &getMap);
    router.any("/api/login", &login);
    router.any("/api/*", &checkLogin);
    router.any("/api/logout", &logout);
    router.any("/api/creategame", &createLobby);
    router.any("/api/game/:game", &getGame);

	listenHTTP(settings, router);
}

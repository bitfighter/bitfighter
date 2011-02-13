/* bitfighter sqlite3 database structure */
/* turn on foreign keys */
PRAGMA foreign_keys = ON;

/* schema */
DROP TABLE IF EXISTS schema;
CREATE  TABLE schema (version INTEGER NOT NULL);
INSERT INTO schema VALUES(1);

/* server */

DROP TABLE IF EXISTS server;
CREATE TABLE server (server_id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL, server_name TEXT, ip_address TEXT  NOT NULL);

CREATE UNIQUE INDEX server_name_ip_unique ON server(server_name COLLATE BINARY, ip_address COLLATE BINARY);

/*  stats_game */
DROP TABLE IF EXISTS stats_game;
CREATE TABLE stats_game (
  stats_game_id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL,
  server_id INTEGER NOT NULL,
  game_type TEXT NOT NULL,
  is_official BOOL NOT NULL,
  player_count INTEGER NOT NULL,
  duration_seconds INTEGER NOT NULL,
  level_name TEXT NOT NULL,
  is_team_game BOOL NOT NULL,
  team_count BOOL NULL,
  insertion_date DATETIME NOT NULL  DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY(server_id) REFERENCES server(server_id));

CREATE INDEX stats_game_server_id ON stats_game(server_id);
CREATE INDEX stats_game_is_official ON stats_game(is_official);
CREATE INDEX stats_game_player_count ON stats_game(player_count);
CREATE INDEX stats_game_is_team_game ON stats_game(is_team_game);
CREATE INDEX stats_game_team_count ON stats_game(team_count);
CREATE INDEX stats_game_game_type ON stats_game(game_type COLLATE BINARY);
CREATE INDEX stats_game_insertion_date ON stats_game(insertion_date);


/* stats_team */
DROP TABLE IF EXISTS stats_team;
CREATE TABLE stats_team (
  stats_team_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  stats_game_id INTEGER NOT NULL,
  team_name TEXT,
  color_hex TEXT NOT NULL,
  team_score INTEGER NULL,
  result CHAR NULL,
  insertion_date DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY(stats_game_id) REFERENCES stats_game(stats_game_id));


CREATE INDEX stats_team_stats_game_id ON stats_team(stats_game_id);
CREATE INDEX stats_team_result  ON stats_team(result COLLATE BINARY);
CREATE INDEX stats_team_insertion_date ON stats_team(insertion_date);


/* stats_player */
DROP TABLE IF EXISTS stats_player;
CREATE TABLE stats_player (
  stats_player_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  stats_game_id INTEGER NOT NULL,
  player_name TEXT NOT NULL,
  is_authenticated BOOL NOT NULL,
  is_robot BOOL NOT NULL,
  result CHAR  NOT NULL,
  points INTEGER NOT NULL,
  kill_count INTEGER  NOT NULL,
  death_count INTEGER  NOT NULL,
  suicide_count INTEGER  NOT NULL,
  switched_team_count INTEGER  NULL,
  stats_team_id INTEGER NULL,
  insertion_date DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY(stats_game_id) REFERENCES stats_game(stats_game_id),
  FOREIGN KEY(stats_team_id) REFERENCES stats_team(stats_team_id));


CREATE INDEX stats_player_is_authenticated ON stats_player(is_authenticated);
CREATE INDEX stats_player_result ON stats_player(result COLLATE BINARY);
CREATE INDEX stats_player_stats_team_id ON stats_player(stats_team_id);
CREATE INDEX stats_player_insertion_date ON stats_player(insertion_date);
CREATE INDEX stats_player_player_name ON stats_player(player_name COLLATE BINARY);
CREATE INDEX stats_player_is_robot ON stats_player(is_robot);
CREATE INDEX stats_player_stats_game_id ON stats_player(stats_game_id);

/* stats_player_shots */
DROP TABLE IF EXISTS stats_player_shots;
CREATE TABLE  stats_player_shots (
  stats_player_shots_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  stats_player_id INTEGER NOT NULL,
  weapon TEXT NOT NULL,
  shots INTEGER NOT NULL,
  shots_struck INTEGER NOT NULL,
  FOREIGN KEY(stats_player_id) REFERENCES stats_player(stats_player_id));


  CREATE UNIQUE INDEX stats_player_shots_player_id_weapon on stats_player_shots(stats_player_id, weapon COLLATE BINARY);


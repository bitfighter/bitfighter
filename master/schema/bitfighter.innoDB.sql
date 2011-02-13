-- phpMyAdmin SQL Dump
-- version 3.3.9
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Feb 12, 2011 at 05:49 PM
-- Server version: 5.1.49
-- PHP Version: 5.3.3

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `bitfighter`
--

-- --------------------------------------------------------

--
-- Table structure for table `server`
--

CREATE TABLE IF NOT EXISTS `server` (
  `server_id` int(11) NOT NULL AUTO_INCREMENT,
  `server_name` text COLLATE utf8_unicode_ci,
  `ip_address` text COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`server_id`),
  UNIQUE KEY `name_ip_unique` (`server_name`(50),`ip_address`(15))
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=9 ;

--
-- Dumping data for table `server`
--


-- --------------------------------------------------------

--
-- Table structure for table `stats_game`
--

CREATE TABLE IF NOT EXISTS `stats_game` (
  `stats_game_id` int(11) NOT NULL AUTO_INCREMENT,
  `server_id` int(11) NOT NULL,
  `game_type` text COLLATE utf8_unicode_ci NOT NULL,
  `is_official` tinyint(1) NOT NULL,
  `player_count` smallint(5) unsigned NOT NULL,
  `duration_seconds` smallint(5) unsigned NOT NULL,
  `level_name` text COLLATE utf8_unicode_ci NOT NULL,
  `is_team_game` tinyint(1) NOT NULL,
  `team_count` tinyint(3) unsigned DEFAULT NULL,
  `insertion_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`stats_game_id`),
  KEY `server_id` (`server_id`),
  KEY `is_official` (`is_official`),
  KEY `player_count` (`player_count`),
  KEY `is_team_game` (`is_team_game`),
  KEY `team_count` (`team_count`),
  KEY `insertion_date` (`insertion_date`),
  KEY `game_type` (`game_type`(20))
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Dumping data for table `stats_game`
--


-- --------------------------------------------------------

--
-- Table structure for table `stats_player`
--

CREATE TABLE IF NOT EXISTS `stats_player` (
  `stats_player_id` int(11) NOT NULL AUTO_INCREMENT,
  `stats_game_id` int(11) NOT NULL,
  `player_name` text COLLATE utf8_unicode_ci NOT NULL,
  `is_authenticated` tinyint(1) NOT NULL,
  `is_robot` tinyint(1) NOT NULL,
  `result` char(1) COLLATE utf8_unicode_ci NOT NULL,
  `points` int(11) NOT NULL,
  `kill_count` smallint(5) unsigned NOT NULL,
  `death_count` smallint(5) unsigned NOT NULL,
  `suicide_count` smallint(5) unsigned NOT NULL,
  `switched_team_count` smallint(5) unsigned DEFAULT NULL,
  `stats_team_id` int(11) DEFAULT NULL,
  `insertion_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`stats_player_id`),
  KEY `is_authenticated` (`is_authenticated`),
  KEY `result` (`result`),
  KEY `stats_team_id` (`stats_team_id`),
  KEY `insertion_date` (`insertion_date`),
  KEY `player_name` (`player_name`(50)),
  KEY `is_robot` (`is_robot`),
  KEY `stats_game_id` (`stats_game_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Dumping data for table `stats_player`
--


-- --------------------------------------------------------

--
-- Table structure for table `stats_player_shots`
--

CREATE TABLE IF NOT EXISTS `stats_player_shots` (
  `stats_player_shots_id` int(11) NOT NULL AUTO_INCREMENT,
  `stats_player_id` int(11) NOT NULL,
  `weapon` text COLLATE utf8_unicode_ci NOT NULL,
  `shots` int(11) NOT NULL,
  `shots_struck` int(11) NOT NULL,
  PRIMARY KEY (`stats_player_shots_id`),
  UNIQUE KEY `stats_player_id` (`stats_player_id`,`weapon`(10)),
  KEY `weapon` (`weapon`(10))
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Dumping data for table `stats_player_shots`
--


-- --------------------------------------------------------

--
-- Table structure for table `stats_team`
--

CREATE TABLE IF NOT EXISTS `stats_team` (
  `stats_team_id` int(11) NOT NULL AUTO_INCREMENT,
  `stats_game_id` int(11) NOT NULL,
  `team_name` text COLLATE utf8_unicode_ci,
  `color_hex` text COLLATE utf8_unicode_ci NOT NULL,
  `team_score` int(11) DEFAULT NULL,
  `result` char(1) COLLATE utf8_unicode_ci DEFAULT NULL,
  `insertion_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`stats_team_id`),
  KEY `stats_game_id` (`stats_game_id`),
  KEY `result` (`result`),
  KEY `insertion_date` (`insertion_date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Dumping data for table `stats_team`
--


--
-- Constraints for dumped tables
--

--
-- Constraints for table `stats_game`
--
ALTER TABLE `stats_game`
  ADD CONSTRAINT `stats_game_ibfk_1` FOREIGN KEY (`server_id`) REFERENCES `server` (`server_id`);

--
-- Constraints for table `stats_player`
--
ALTER TABLE `stats_player`
  ADD CONSTRAINT `stats_player_ibfk_2` FOREIGN KEY (`stats_team_id`) REFERENCES `stats_team` (`stats_team_id`),
  ADD CONSTRAINT `stats_player_ibfk_3` FOREIGN KEY (`stats_game_id`) REFERENCES `stats_game` (`stats_game_id`);

--
-- Constraints for table `stats_player_shots`
--
ALTER TABLE `stats_player_shots`
  ADD CONSTRAINT `stats_player_shots_ibfk_1` FOREIGN KEY (`stats_player_id`) REFERENCES `stats_player` (`stats_player_id`);

--
-- Constraints for table `stats_team`
--
ALTER TABLE `stats_team`
  ADD CONSTRAINT `stats_team_ibfk_1` FOREIGN KEY (`stats_game_id`) REFERENCES `stats_game` (`stats_game_id`);

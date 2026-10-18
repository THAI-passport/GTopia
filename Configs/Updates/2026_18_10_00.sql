ALTER TABLE Bans ADD COLUMN PunishType int DEFAULT '0';
ALTER TABLE Bans MODIFY COLUMN ModType int DEFAULT '0';
ALTER TABLE Bans MODIFY COLUMN Reason char(255) DEFAULT '';
ALTER TABLE Bans ADD COLUMN Mac CHAR(24) DEFAULT NULL, ADD INDEX `idx_mac` (`Mac`);
ALTER TABLE Players ADD COLUMN PlayTimeSeconds int UNSIGNED NOT NULL DEFAULT '0';

CREATE TABLE `Holidays` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `HolidayKey` varchar(64) NOT NULL,
  `DurationDays` int UNSIGNED NOT NULL DEFAULT '1',
  `Type` tinyint NOT NULL DEFAULT '0',
  `StartTimeUnix` int UNSIGNED NOT NULL DEFAULT '0',
  `IsActive` tinyint NOT NULL DEFAULT '0',
  `ScheduleType` tinyint NOT NULL DEFAULT '0',
  `ScheduleParam` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`ID`),
  UNIQUE KEY `HolidayKey` (`HolidayKey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `Holidays` (`HolidayKey`, `DurationDays`, `Type`, `StartTimeUnix`, `IsActive`, `ScheduleType`, `ScheduleParam`) VALUES ('comet', 1, 1, 1791401155, 0, 2, '28');

CREATE TABLE `HolidayRules` (
  `ID` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `HolidayKey` varchar(64) NOT NULL,
  `RuleType` tinyint NOT NULL DEFAULT '0',
  `StatKey` varchar(64) NOT NULL,
  `OperatorType` tinyint NOT NULL DEFAULT '0',
  `Threshold` int NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `Stats` (
  `StatKey` varchar(64) NOT NULL,
  `StringVal` varchar(255) DEFAULT NULL,
  `FloatVal` float DEFAULT '0',
  `IntVal` int DEFAULT '0',
  `UpdatedAt` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `Category` tinyint NOT NULL DEFAULT '0',
  PRIMARY KEY (`StatKey`),
  KEY `idx_category` (`Category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
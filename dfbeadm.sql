-- SQL script to generate the DFBEADM(8)
-- filesystem record database

-- Database setting via the PRAGMA command in SQLite3
PRAGMA cell_size_check=true;
PRAGMA case_sensitive_like=true;
PRAGMA secure_delete=true;
PRAGMA foreign_keys=on;

-- Database and Application version info
-- NOTE: These are currently placeholders
PRAGMA user_version=0;
PRAGMA application_id=0;

-- Table dofinitions
-- Pseudo-ENUM key/value table
CREATE TABLE IF NOT EXISTS hashalgo (
	id integer UNIQUE NOT NULL, -- Internal use hash algo ID
	algo text UNIQUE NOT NULL, -- Human friendly name of the hash algo
	PRIMARY KEY (id,algo)
);

CREATE TABLE IF NOT EXISTS h2be (
	belabel text UNIQUE NOT NULL, -- Should be usable as a primary key
	fstab blob NOT NULL, -- Hold the complete fstab
	active bool NOT NULL DEFAULT false, -- Is this the one currently in use?
	extant bool NOT NULL DEFAULT true, -- Does this boot environment still exist?
	fshash text UNIQUE NOT NULL, -- Since belabel is unique, this should also be unique
	hashspec integer NOT NULL DEFAULT 0, -- Default to using whirlpool for the fstab digest
	PRIMARY KEY (belabel,fshash),
	FOREIGN KEY (hashspec) REFERENCES hashalgo(id)
);

-- Index creation to help prevent slow lookups
CREATE INDEX if NOT EXISTS extant_bootenvs ON h2be (belabel,extant);

-- Populate the hash algo table with hashes 
-- Not all of these are currently available, but should all be 
-- resonably good hashes with good performance.
-- These are probably overkill for simple integrity checking, but 
-- to my knowledge have no risk of collisions, 
-- allowing for more expedient verification that the database and disk agree
BEGIN;
	INSERT INTO hashalgo VALUES
	(0, 'whirlpool'),
	(1, 'sha3-512'), -- Not in LibreSSL 2.9.1
	(2, 'blake2b512'), -- Not in LibreSSL 2.9.1
	(3, 'shake256'), -- Not in LibreSSL 2.9.1
	(4, 'sha2-512');
COMMIT;

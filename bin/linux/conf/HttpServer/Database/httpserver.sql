
CREATE TABLE scriptitem_t
(
	filename 			VARCHAR(256),
	code					VARCHAR(64),
	sub						SMALLINT,
	itemtype			INT,
	object1				INT,
	object2				INT,
	id						VARCHAR(32),
	scopy					VARCHAR(16),
	name					VARCHAR(32),
	property			VARCHAR(32),
	type					VARCHAR(16),
	value					CLOB,
	vlen					INT DEFAULT '0'
);
CREATE INDEX scriptitem_t_idx_filename ON scriptitem_t(filename);

CREATE TABLE cspfileinfo_t
(
	filename 			VARCHAR(256) PRIMARY KEY,
	filesize			BIGINT,
	lasttime			BIGINT
);


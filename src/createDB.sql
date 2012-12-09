BEGIN;

CREATE TABLE namespace (prefix TEXT, uri TEXT, fixed INTEGER);
INSERT INTO namespace VALUES('rdf',  'http://www.w3.org/1999/02/22-rdf-syntax-ns#', 1);
INSERT INTO namespace VALUES('rdfs', 'http://www.w3.org/2000/01/rdf-schema#', 1);
INSERT INTO namespace VALUES('owl',  'http://www.w3.org/2002/07/owl#', 1);
INSERT INTO namespace VALUES('xsd',  'http://www.w3.org/2001/XMLSchema#', 1);
INSERT INTO namespace VALUES('dc',   'http://purl.org/dc/elements/1.1/', 1);
INSERT INTO namespace VALUES('sib',  'http://www.nokia.com/NRC/M3/sib#', 1);
INSERT INTO namespace VALUES('daml', 'http://www.daml.org/2000/12/daml+oil#', 0);

CREATE TABLE node (id INTEGER UNIQUE PRIMARY KEY, str TEXT, datatype INTEGER, lang TEXT);
CREATE INDEX strs ON node (str);
INSERT INTO node VALUES(1, 'http://www.w3.org/1999/02/22-rdf-syntax-ns#type', 0, NULL);
INSERT INTO node VALUES(2, 'http://www.w3.org/1999/02/22-rdf-syntax-ns#Property', 0,
       	    	        NULL);
INSERT INTO node VALUES(3, 'http://www.w3.org/2000/01/rdf-schema#Resource', 0, NULL);
INSERT INTO node VALUES(4, 'http://www.w3.org/2000/01/rdf-schema#Class', 0, NULL);
INSERT INTO node VALUES(5, 'http://www.w3.org/2000/01/rdf-schema#subClassOf', 0, NULL);
INSERT INTO node VALUES(6, 'http://www.w3.org/2000/01/rdf-schema#label', 0, NULL);
INSERT INTO node VALUES(0, 'http://www.nokia.com/NRC/M3/sib#any', 0, NULL);

CREATE TABLE triple (s INTEGER, p INTEGER, o INTEGER, src INTEGER);
CREATE INDEX s ON triple (s);
CREATE INDEX o ON triple (o);
CREATE INDEX sp ON triple (s, p);
CREATE INDEX spo ON triple (s, p, o);
CREATE INDEX po ON triple (p, o);

CREATE TABLE source (src INTEGER UNIQUE PRIMARY KEY, created INTEGER, loaded INTEGER);

CREATE TABLE info (version TEXT);
INSERT INTO info VALUES('Piglet 0.1');

COMMIT;

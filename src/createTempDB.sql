CREATE TABLE cache.bnode (id INTEGER UNIQUE PRIMARY KEY, str TEXT);
CREATE INDEX cache.strs ON bnode (str);

CREATE TABLE cache.triple (s INTEGER, p INTEGER, o INTEGER, src INTEGER);
CREATE INDEX cache.s ON triple (s);
CREATE INDEX cache.o ON triple (o);
CREATE INDEX cache.sp ON triple (s, p);
CREATE INDEX cache.spo ON triple (s, p, o);
CREATE INDEX cache.po ON triple (p, o);

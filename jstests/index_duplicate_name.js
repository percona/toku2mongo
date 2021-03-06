var d = db.getSiblingDB(jsTestName());
var c = d.foo;
c.drop();
c.insert({x: 1, y: 1});
assert.eq(1, c.count());
c.ensureIndex({x: 1}, {name: "collide"});
assert.eq(2, c.getIndexes().length);
c.ensureIndex({y: 1}, {name: "collide"});
assert.eq(2, c.getIndexes().length);
assert.eq(1, c.count());

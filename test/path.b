print "---- make"
p: make path! [a 2]
print [type? p p]


a: ["abc" "hij" "xyz"]
print reduce [make path! [a 2]]
probe a/1
probe a/4


print "---- lit-path"
p: 'some/path
print [type? p p]
lp: 'a/'b
print [type? lp lp type? first lp type? second lp]


print "---- set-path"
blk: [a [0 (0)]]
blk/2/1: 5
blk/2/2/1: 'pword
probe blk
n: skip p: [1 2 3] 2
n/1: 'new
probe p


print "---- block"
a: [x 33 y: 44]
print [a/x a/y]
blk: [a 1 b [x 11 y (aa bb cc)]]
probe blk/b/y/3
blk/b/y/3: 44
probe blk


print "---- integer"
a: first [foo/-900/48]
foreach elem a [
    print [:elem type? elem]
]


print "---- get-word"
ctx: context [a: 1 b: 2]
val: 'b
probe ctx/:val
blk: [foo [bar sol] b cog]
probe blk/:val
val: 2
probe blk/:val/2

(def {nil} {})
(def {true} 1)
(def {false} 0)

(def {fun} (\ {decl body} {
    def (head decl) (\ (tail decl)  body) 
}))

(fun {unpack f l} {
  eval (join (list f) l)
})

(fun {pack f & xs} {f xs})

(def {curry} unpack)
(def {uncurry} pack)

(fun {do & l} {
  if (== l nil)
    {nil}
    {last l}
})

(fun {let b} {
  ((\ {_} b) ())
})


(fun {not x} {- 1 x})
(fun {and x y} {* x y})
(fun {or x y} {+ x y})

(fun {flip f a b} {f b a})
(fun {ghost & xs} {eval xs})
(fun {comp f g x} {f (g x)})


(fun {fst l} {eval (head l)})
(fun {snd l} {eval (head (tail l))})
(fun {trd l} {eval (head (tail (tail l)))})

(fun {len l} {
  if (== l nil)
    {0}
    {+ 1 (len (tail l))}
})

(fun {nth n l} {
  if (== n 0)
    {fst l}
    {nth (- n 1) (tail l)}
})

(fun {last l} {nth (- (len l) 1) l})

(fun {take n l} {
  if (== n 0)
    {nil}
    {join (head l) (take (- n 1) l)}
})

(fun {drop n l} {
  if (== n 0)
    {l}
    {drop (- n 1) l}
})

(fun {split n l} {list (take n l) (drop n l)})

(fun {elem x l} {
  if (== l nil)
    {false}
    {if (== x (fst l)) {true} {elem x (tail l)}}
})

(fun {map f l} {
  if (== l nil)
    {nil}
    {join (list (f (fst l))) (map f (tail l))}
})

(fun {filter f l} {
  if (== l nil)
    {nil}
    {join (if (f (fst l)) {head l} {nil}) (filter f (tail l))}
})

(fun {foldl f z l} {
  if (== l nil)
    {z}
    {foldl f (f z (fst l)) (tail l)}
})

(fun {sum l} {foldl + 0 l})
(fun {product l} {foldl * 1 l})


(fun {select & cs} {
  if (== cs nil)
    {error "No selection Found"}
    {if (fst (fst cs)) {snd (fst cs)} {unpack select (tail cs)}}
})

(def {otherwise} true)


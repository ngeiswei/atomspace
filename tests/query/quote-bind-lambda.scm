(define ll
(LambdaLink
   (VariableList
      (VariableNode "$GSN")
      (VariableNode "$ARG")
   )
   (QuoteLink
      (ExecutionOutputLink
         (UnquoteLink (VariableNode "$GSN"))
         (UnquoteLink (VariableNode "$ARG"))
      )
   )
)
)

(define bl
(BindLink
   (EvaluationLink
      (GroundedPredicateNode "scm: dummy")
      ll
   )
   (Concept "OK")
)
)

(define (dummy A)
  (stv 1 1))

(define expected (Concept "OK"))

;; Experiment
(define il
(Implication
   (And
      (Variable "$GSN")
      (Variable "$ARG"))
   ll)
)

(define pl
(Put
   (VariableList
      (Variable "$GSN")
      (Variable "$ARG")
   )
   il)
)

(define il-expected
(Implication
   (And
      (Predicate "P")
      (Predicate "Q"))
   ll)
)

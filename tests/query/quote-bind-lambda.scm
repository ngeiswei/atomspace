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
      (Quote ll)
   )
   (Concept "OK")
)
)

(define (dummy A)
  (stv 1 1))

(define expected (Set (Concept "OK")))

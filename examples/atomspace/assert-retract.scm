;
; assert-retract.scm -- Asserting and retracting facts.
;
; A knowledgebase is rarely static. one needs to be able to assert
; new facts, and retract old ones.  Of course, one can always do this
; directly: just write some Atoms at the guile prompt, or just delete
; them (using the `cog-delete` function). But what if you do not yet
; know what these facts are? What if they are the result of logical
; reasoning?
;
; Rather than writing a procedural program that performs reasoning,
; and then just performs the insertion/deletion "manually" (by calling
; some function "proceedurally"), one can also maintain the knowlegbase
; with knowledge-snippets themselves. That is, the tools for maintaining
; the data can be a part of the data itself.
;
; This is done with two link types: GetLink and PutLink. The GetLink
; is another form of query, much like BindLink, but without the
; re-writing: it is "half" of an implication. PutLink is the other half
; of the implication: it only does the re-writing part. PutLink is
; a form of beta-reduction for graphs.  These two halves can be
; combined; thus, every BindLink is equivalent to a Get-Put pair.
;
; Splitting graph-query and rewriting into two pieces allows for greater
; flexibility in how knowledge self-mutates at runtime.  The example
; below
; allows 
; The cog-execute! function is used to assert facts, or retract them
; from the AtomSpace.  The idea of asserting and retracting facts is
; taken from ProLog, where the system as a whole behave like a database,
; and there must be a way of adding records, or removing them from the
; database.  So, likewise, in the AtomSpace: the AtomSpace is a database,
; and the PutLink and DeleteLink provide a way to add and remove
; statements when they are executed.

(use-modules (opencog))
(use-modules (opencog exec))
(use-modules (opencog query))

; A utility function to print all EvaluationLinks in the AtomSpace.
(define (show-eval-links)
	(cog-map-type (lambda (h) (display h) #f) 'EvaluationLink))

; The EvaluationLink won't be added until this is reduced.
; When it is reduced, the ListLink will be substitited for the
; variable $x, creating the fully-assembled EvaluationLink.
(define to-be-added
	(PutLink
		(EvaluationLink
		    (PredicateNode "some property")
          (VariableNode "$x"))
		(ListLink
			(ConceptNode "thing A")
			(ConceptNode "B-dom-ness"))))

; Verify that the atomspace contains no EvaluationLinks:
(show-eval-links)

; Now, actually create the EvaluationLink.
(cog-execute! to-be-added)

; Take a look again:
(show-eval-links)

; One way to view the result of having run the PutLink is to
; use the GetLink with the same pattern.  Thus, the GetLink
; below has a satisfying set that corresponds to the PutLink
; above.

(define get-value
	(GetLink
		(EvaluationLink
			(PredicateNode "some property")
			(VariableNode "$x"))))

; The cog-execute! function will return the value(s) that
; the GetLink finds.  If only one value satsifies the query, then
; that is returned. Else a SetLink is returned. Equivalently,
; the cog-execute! function will do the same thing.
(cog-execute! get-value)
(cog-execute! get-value)

; The PutLink below causes the put-link above to be un-done.
; It explicitly specifies the same parts as were specified above,
; but when these are assembled, it causes the DeleteLink to
; run and remove them.  That is, it is impossible to insert
; a DeleteLink into the atomspace, if it does not have any
; variables in it. Attempting such an insertion will cause the
; body of the DeleteLink to be removed.
(define remove-thing-ab
	(PutLink
		(DeleteLink
			(EvaluationLink
				(PredicateNode "some property")
				(VariableNode "$x")))
		(ListLink
			(ConceptNode "thing A")
			(ConceptNode "B-dom-ness"))))

; Force its removal.
(cog-execute! remove-thing-ab)

; Look for it; it should be absent.
(cog-execute! get-value)
; Double-check it's absence.
(show-eval-links)

; Add it back in:
(cog-execute! to-be-added)
(cog-execute! get-value)

; ... and so on. We can now continue to remove it and add it
; back in repeatedly.
(cog-execute! remove-thing-ab)
(cog-execute! get-value)
(cog-execute! to-be-added)
(cog-execute! get-value)


; It is also useful to generically remove any atom matching
; a pattern description. This can be done by combining the
; PutLink with a GetLink performing a query. The below uses
; the GetLink to find groundings for the variable $x, and then
; passes those groundings to the PutLink/DeleteLink combination,
; which removes them.
;
(define remove-some-property
	(PutLink
		(DeleteLink
			(EvaluationLink
				(PredicateNode "some property")
				(VariableNode "$x")))
		(GetLink
			(EvaluationLink
				(PredicateNode "some property")
				(VariableNode "$x")))))

; Now, remove the EvaluationLink
(cog-execute! remove-some-property)
(cog-execute! get-value)

; We can now add and remove over and over:
(cog-execute! to-be-added)
(cog-execute! get-value)

(cog-execute! remove-some-property)
(cog-execute! get-value)

; And do it again, for good luck:
(cog-execute! to-be-added)
(cog-execute! get-value)
(cog-execute! remove-some-property)
(cog-execute! get-value)


; ------------------------------------------------
; The simplest way to combine Delete/Get/Put to maintain state is to
; use the StateLink.  StateLinks do not even have to be executed;
; simply using them changes the state. See state.scm for details.
;
; Thus for example:

(StateLink
	(PredicateNode "some property")
	(ListLink
		(ConceptNode "thing A")
		(ConceptNode "alternative B")))

(define get-state
	(GetLink
		(StateLink
			(PredicateNode "some property")
			(VariableNode "$x"))))

(cog-execute! get-state)

(StateLink
	(PredicateNode "some property")
	(ListLink
		(ConceptNode "thing A")
		(ConceptNode "The V alternative")))

(cog-execute! get-state)

(StateLink
	(PredicateNode "some property")
	(ListLink
		(ConceptNode "thing A")
		(ConceptNode "first alternative again")))

(cog-execute! get-state)

; ... and so on, ad infinitum

; ------------------------------------------------
; DefineLink can be used to specify the body of a PutLink.
; Thus, for example:

(DefineLink
	(DefinedSchemaNode "colored things")
	(LambdaLink
		(InheritanceLink
			(ConceptNode "color")
			(VariableNode "$yyy"))))

(cog-execute!
	(PutLink
		(DefinedSchemaNode "colored things")
		(ConceptNode "green")))

; Will cause the following to be created:
;
; (InheritanceLink
;    (ConceptNode "color")
;    (ConceptNode "green"))

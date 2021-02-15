;
; file-utils.scm
; Assorted file and directory utils.
;
; The following utilities are provided in this file:
;
; -- list-files dir                List files in a directory
; -- exec-scm-from-port port       Execute scheme code read from port
; -- exec-scm-from-cmd cmd-string  Run scheme returned by shell command
; -- load-scm-from-file filename   Run scheme code taken from file
; -- export-all-atoms filename     Export entire atomsapce into file
;
; Copyright (c) 2008, 2013 Linas Vepstas
;

(use-modules (ice-9 rdelim))
(use-modules (ice-9 popen))
(use-modules (ice-9 rw))
(use-modules (rnrs io ports))

; ---------------------------------------------------------------------
(define-public (list-files dir)
"
 list-files DIR    List files in directory DIR

 Given a directory, return a list of all of the files in the directory
 Do not return anything that is a subdirectory, pipe, special file etc.
"

	(define (isfile? file)
		(eq? 'regular (stat:type (stat
				(string-join (list dir file) "/")))
		)
	)

	; suck all the filenames off a port
	(define (suck-in-filenames port lst)
		(let ((one-file (readdir port)))
			(if (eof-object? one-file)
				lst
				(suck-in-filenames port
					(if (isfile? one-file)
						(cons one-file lst)
						lst
					)
				)
			)
		)
	)
	(let* ((dirport (opendir dir))
			(filelist (suck-in-filenames dirport '()))
		)
		(closedir dirport)
		filelist
	)
)

; ---------------------------------------------------------------------
(define-public (exec-scm-from-port port)
"
 exec-scm-from-port PORT   Execute scheme code read from PORT

 Read (UTF-8 encoded) data from the indicated port, and run it.
 The port should contain valid scheme; this routine will read and
 execute that scheme data.

 CAUTION: This routine will hang until the remote end closes the port.
 That is, it will continue to attempt to read more data from the port,
 until an EOF is received.  For sockets, an EOF is sent only when the
 remote end closes its transmit port.

 This is just a wrapper around guile's native `eval-string`, and thus
 will not be very fast, if you just want to load Atomese. For fast
 Atomesse import, use the `(use-modules (opencog persist-file))`
 module, and load from file as `(load-file \"/tmp/x.scm\")`

 See also: `exec-scm-from-cmd`
"

	; get-string-all is a new r6rs proceedure, sucks in all bytes until
	; EOF on the port. Seems like TCP/IP ports end up being textual in
	; guile, and the default r6rs transcoder is UTF8 and so everyone
	; is happy, these days.  Note: in the good-old bad days, we used
	; ice-9 rw read-string!/partial for this, which went buggy, and
	; started mangling at some point.
	(let ((string-read (get-string-all port)))
		(if (eof-object? string-read)
			#f
			(eval-string string-read)
		)
	)
)

; ---------------------------------------------------------------------
(define-public (exec-scm-from-cmd cmd-string)
"
 exec-scm-from-cmd CMD-STRING   Run scheme returned by shell command

 Load data generated by the system command CMD-STRING. The command
 should generate valid scheme, and send its data to stdout; this
 routine will read and execute that scheme data.

 Example:
 (exec-scm-from-cmd \"cat /tmp/some-file.scm\")
 (exec-scm-from-cmd \"cat /tmp/some-file.txt | perl ./bin/some-script.pl\")

 This is just a wrapper around guile's native `eval-string`, and thus
 will not be very fast, if you just want to load Atomese. For fast
 Atomesse import, use the `(use-modules (opencog persist-file))`
 module, and load from file as `(load-file \"/tmp/x.scm\")`

 See also: `exec-scm-from-port`
"

	(let ((port (open-input-pipe cmd-string)))
		(exec-scm-from-port port)
		(close-pipe port)
	)
)

; ---------------------------------------------------------------------
; XXX this duplicates (load-from-path) which is a built-in in guile...
(define-public (load-scm-from-file filename)
"
 load-scm-from-file filename   Run scheme code taken from file

 Load scheme data from the indicated file, and execute it.

 Example Usage:
 (load-scm-from-file \"/tmp/some-file.scm\")

 DEPRECATED! This just duplicates the functionality of the guile
 built-in functions `load`, `load-from-path`, `primitive-load` and
 `primitive-load-from-path`. Worse, its slow. If you just want to
 load Atomese, and do it quickly, use the
 `(use-modules (opencog persist-file))`
 module, and load from file as `(load-file \"/tmp/x.scm\")`
"
	(exec-scm-from-cmd (string-join (list "cat \"" filename "\"" ) ""))
)

; ---------------------------------------------------------------------
(define-public (prt-atom-list port lst)
"
 prt-atom-list PORT LST     Print a list LST of atoms to PORT.

 Prints the list of atoms LST to PORT, but only those atoms
 without an incoming set.

 See also: `cog-prt-atomspace`.
"
	(for-each
		(lambda (atom)
			(if (null? (cog-incoming-set atom)) (display atom port)))
		lst)
)

; ---------------------------------------------------------------------
(define-public (export-atoms lst filename)
"
 export-atoms LST FILENAME   Export the atoms in LST to FILENAME.

 Exports the list of atoms to the file 'FILENAME'. If an absolute/relative
 path is not specified, then the filename will be written to the directory
 in which the opencog server was started.

 See also `export-all-atoms`, `prt-atom-list`, `cog-prt-atomspace`.
"
    (let ((port (open-file filename "w")))
        (map (lambda (atom) (display atom port)) lst)
        (close-port port)
    )
)

; ---------------------------------------------------------------------
(define-public (export-all-atoms filename)
"
 export-all-atoms FILENAME    Export entire atomspace into file

 Export the entire contents of the atomspace to the file 'FILENAME'
 If an absolute path is not specified, then the filename will be
 written to the directory in which the opencog server was started.

 See also: `export-atoms`, `prt-atom-list`, `cog-prt-atomspace`.
"
	(let ((port (open-file filename "w"))
		)
		(for-each
			(lambda (x) (prt-atom-list port (cog-get-atoms x)))
			(cog-get-types)
		)

		(close-port port)
	)
)

; ---------------------------------------------------------------------

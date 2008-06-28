; Recompile.ss: Drives the recompilation and execution of code

(begin
; Code fragments (basic blocks) hash tables of functions
;   which begin at the address represented by the key
(define fragments (make-hash-table))

(define (invalidate-range start end)
  (hash-table-for-each fragments
                       (lambda (k v)
                         (if (and (>= k start) (< k end))
                             (hash-table-remove! fragments k)))))

; Program counter closure
(define (set-pc) (raise "set-pc not properly defined"))
(define (get-pc) (raise "get-pc not properly defined"))
(define (get-next-src) (raise "get-next-src not properly defined"))
(let ((pc 0))
  (set! set-pc (lambda (new-value) (set! pc new-value)))
  (set! get-pc (lambda () pc))
  (set! get-next-src (lambda () (umemw pc))))

; FIXME: maybe branch should (raise '(end-of-frag last-op))
;          and (with-handlers (((lambda (e) (eq? 'end-of-frag (car e)) (hash...)))) 
(define end-of-frag #f)
(define (recompile-fragment start)
  ; Set up the environment
  (set-pc start)
  (let loop ((frag ()))
    (with-handlers (((lambda (e) (eq? 'end-of-frag (car e)))
                     (lambda (e)
                       (hash-table-put! fragments start
                                        `(begin ,(nreverse (cons (cdr e) frag)))))))
       (loop (cons (gen-op (get-next-src)) frag)))))
  #| Old method: used a global flag
  (set! end-of-frag #f)
  ; Build up the fragment
  (let loop ((frag ()))
    (if end-of-frag
        ; If we've reached the end, reverse our list and encapsulate it in a begin
        (hash-table-put! fragments start `(begin ,(nreverse frag)))
        ; Otherwise, just cons our new 'instruction' and keep going
        (loop (cons (gen-op (get-next-src)) frag)))))
  |#

(define (execute pc)
  ; Get the fragment from the hash-table (recompiling on failure)
  (let ((frag (hash-table-get fragments pc 
                              (lambda () (begin (recompile-fragment pc) #f)))))
    ; If the frag was already there, we can just eval it
    (if frag
        (eval frag)
        ; Otherwise, it should live in the hash now since we recompiled
        (eval (hash-table-get fragments pc)))))
    

(define (dynarec start)
  (let loop ((pc start))
    (if (stop-signaled?)
        pc
        (loop (execute pc)))))

)

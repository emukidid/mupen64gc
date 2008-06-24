; Recompile.ss: Drives the recompilation and execution of code

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
  (set! get-next-src (lambda () (raise "TODO: implement get-next-src"))))

(define end-of-frag #f)
(define (recompile-fragment start)
  ; Set up the environment
  (set-pc start)
  (set! end-of-frag #f)
  ; Build up the fragment
  (let loop ((frag ()))
    (if end-of-frag
        ; If we've reached the end, reverse our list and encapsulate it in a begin
        (hash-table-put! fragments start `(begin ,(nreverse frag)))
        ; Otherwise, just cons our new 'instruction' and keep going
        (loop (cons (gen-op (get-next-src)) frag)))))

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
    (if stop
        pc
        (loop (execute pc)))))

    
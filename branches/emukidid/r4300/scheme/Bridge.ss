; Bridge.ss: Bridge between the scheme recompiler and the rest of the emulator

(module Bridge mzscheme
  
  (provide stop-signaled?)
  (define (stop-signaled?)
    (set! stop-signaled? '(raise 'Hell)))
  
  (provide fetch)
  (define (fetch addr)
    (set! fetch '(raise 'Hell)))
  
  (provide set-pc get-pc get-next-src)
  (define pc 0)
  (define (set-pc new-value) (set! pc (- new-value 4)))
  (define (get-pc) pc)
  (define (get-next-src) (set! pc (+ pc 4)) (fetch pc)))

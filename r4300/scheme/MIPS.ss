; MIPS.ss: Handlers for MIPS instructions (generates scheme code for each instruction)
; by Mike Slegeir for Mupen64-GC


(module MIPS mzscheme
  (require mzscheme)
  (require (lib "defmacro.ss"))
  (require "Bridge.ss")
  (provide gen-op)

  ; TODO: functions which probably need to be implemented in C code somewhere:
  (define (s16-ext val)
    (if (= (bitwise-and val #x80) #x80)
        (- (+ (bitwise-and (bitwise-not val) #xFF) 1))
        val))
  ; FIXME: Actually make this one logical
  (define logical-shift arithmetic-shift)
  (define (interpret instr)
    (raise `(end-of-frag (decode&interpret ,instr ,(get-pc)))))
    ;(set! end-of-frag #t)
    ;(decode&interpret instr))

  ; Hash tables for decoding instructions
  (define opcode-handlers (make-hash-table))
  (define special-handlers (make-hash-table))

  ; Entry-point: given the instruction, we find the opcode and return its implementation
  (define (gen-op instr)
    (printf "gen-op ~X~%" instr)
    ((hash-table-get opcode-handlers
                     (bitwise-and (arithmetic-shift instr -26) #x3F) (lambda () NI))
     instr))
  ;(define (gen-delay)
  ;  (gen-op (get-next-src)))


  ; General functions
  (define (branch addr)
    (raise `(end-of-frag ,addr)))
    ;(set! end-of-frag #t)
    ;addr)

  ; Macros for extracting fields of an instruction
  (define-macro (get-rt instr)
    `(bitwise-and (arithmetic-shift ,instr -16) #x1F))
  (define-macro (get-rs instr)
    `(bitwise-and (arithmetic-shift ,instr -21) #x1F))
  (define-macro (get-rd instr)
    `(bitwise-and (arithmetic-shift ,instr -11) #x1F))
  (define-macro (get-immed instr)
    `(bitwise-and ,instr #xFFFF))
  (define-macro (get-sa instr)
    `(bitwise-and (arithmetic-shift ,instr -6) #x1F))
  (define-macro (get-li instr)
    `(bitwise-and ,instr #x3FFFFFF))
  ; Form macros
  (define-macro (opcode number name body)
    `(begin
       (define ,name (lambda (instr) ,body))
       (hash-table-put! opcode-handlers ,number ,name)))
  (define-macro (special number name body)
    `(begin
       (define ,name (lambda (instr) ,body))
       (hash-table-put! special-handlers ,number ,name)))

  (define-macro (with-i-form body)
    `(let ((rt ,`(get-rt instr))
           (rs ,`(get-rs instr))
           (immed ,`(get-immed instr)))
       ,body))
  (define-macro (with-shift-form body)
    `(let ((rd ,`(get-rd instr))
           (rt ,`(get-rt instr))
           (sa ,`(get-sa instr)))
       ,body))
  (define-macro (with-reg-form body)
    `(let ((rd ,`(get-rd instr))
           (rt ,`(get-rt instr))
           (rs ,`(get-rs instr)))
       ,body))
  (define-macro (with-mult-form body)
    `(let ((rt ,`(get-rt instr))
           (rs ,`(get-rs instr)))
       ,body))
  (define-macro (with-regimm-form body)
    `(let ((rs ,`(get-rs instr))
           (immed ,`(get-immed instr)))
       ,body))

  ; Instruction implementations
  (define (NI instr)
    (printf "instruction ~X not implemented!~%" instr) 
    (interpret instr))

  (opcode #x00 SPECIAL-OPCODE
          ((hash-table-get special-handlers
                           (bitwise-and instr #x3F) (lambda () NI))
           instr))

  ; TODO: REGIMM
#|
(opcode #x02 J
  (let ((li (get-li instr)))
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; Branch to ($PC & 0xF0000000) | (li << 2)
       (branch ,(bitwise-or 
                  (bitwise-and (get-pc) #xF0000000) 
                  (arithmetic-shift li 2))))))

(opcode #x03 JAL
  (let ((li (get-li instr)))
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; Link
       (r= 31 ,(+ (get-pc) 4))
       ; Branch to ($PC & 0xF0000000) | (li << 2)
       (branch ,(bitwise-or
                  (bitwise-and (get-pc) #xF0000000)
                  (arithmetic-shift li 2))))))

(opcode #x04 BEQ
  (with-i-form
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; If $rs = $rt
       (if (= (reg ,rs) (reg ,rt))
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2)))
           ; Else continue executing past the delay slot
           (branch ,(+ (get-pc) 8))))))

(opcode #x05 BNE
  (with-i-form
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; If $rs = $rt
       (if (= (reg ,rs) (reg ,rt))
           ; Continue executing past the delay slot
           (branch ,(+ (get-pc) 8))
           ; Else branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2)))))))

(opcode #x06 BLEZ
  (with-regimm-form
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; If $rs <= 0
       (if (<= (reg ,rs) 0)
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2)))
           ; Else continue executing past the delay slot
           (branch ,(+ (get-pc) 8))))))

(opcode #x07 BGTZ
  (with-regimm-form
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; If $rs > 0
       (if (> (reg ,rs) 0)
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2)))
           ; Else continue executing past the delay slot
           (branch ,(+ (get-pc) 8))))))
|#

  (opcode #x09 ADDIU
          (with-i-form
           `(r= ,rt (+ (reg ,rs) ,(s16-ext immed)))))

  (define ADDI ADDIU)
  (hash-table-put! opcode-handlers #x08 ADDI)

  (opcode #x0a SLTI
          (with-i-form
           `(r= ,rt (if (< (reg ,rs) ,(s16-ext immed)) 1 0))))

  (opcode #x0b SLTIU
          (with-i-form
           `(r= ,rt (if (< (ureg ,rs) ,immed)) 1 0)))

  (opcode #x0c ANDI
          (with-i-form
           `(r= ,rt (bitwise-and (reg ,rs) ,immed))))

  (opcode #x0d ORI
          (with-i-form
           `(r= ,rt (bitwise-or (reg ,rs) ,immed))))

  (opcode #x0e XORI
          (with-i-form
           `(r= ,rt (bitwise-xor (reg ,rs) ,immed))))

  (opcode #x0f LUI
          (with-i-form
           `(r= ,rt (arithmetic-shift ,immed 16))))

  ; TODO: COP0   , COP1

#|
(opcode #x14 BEQL
  (with-i-form
    ; If $rs = $rt
    `(if (= (reg ,rs) (reg ,rt))
         ; Do:
         (begin
           ; Recompile the delay slot
           ,(gen-delay)
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2))))
         ; Else continue executing past the delay slot
         (branch ,(+ (get-pc) 8)))))

(opcode #x15 BNEL
  (with-i-form
    ; If $rs = $rt
    `(if (= (reg ,rs) (reg ,rt))
         ; Continue executing past the delay slot
         (branch ,(+ (get-pc) 8))
         ; Else do:
         (begin
           ; Recompile the delay slot
           ,(gen-delay)
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2)))))))

(opcode #x16 BLEZL
  (with-regimm-form
    ; If $rs <= 0
    `(if (<= (reg ,rs) 0)
         ; Do:
         (begin
           ; Recompile the delay slot
           ,(gen-delay)
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2))))
         ; Else continue executing past the delay slot
         (branch ,(+ (get-pc) 8)))))

(opcode #x17 BGTZL
  (with-regimm-form
    ; If $rs > 0
    `(if (> (reg ,rs) 0)
         ; Do:
         (begin
           ; Recompile the delay slot
           ,(gen-delay)
           ; Branch by immed << 2
           (branch ,(+ (get-pc) (arithmetic-shift (s16-ext immed) 2))))
         ; Else continue executing past the delay slot
         (branch ,(+ (get-pc) 8)))))
|#

  (opcode #x19 DADDIU
          (with-i-form
           `(r64= ,rt (+ (reg64 ,rs) ,(s16-ext immed)))))

  (define DADDI DADDIU)
  (hash-table-put! opcode-handlers #x18 DADDI)

#|
; TODO: LDL, LDR

(opcode #x20 LB
  (with-i-form
    `(r= ,rt (s8-ext (memb (+ (reg ,rs) ,(s16-ext immed)))))))

(opcode #x21 LH
  (with-i-form
    `(r= ,rt (s16-ext (memh (+ (reg ,rs) ,(s16-ext immed)))))))

; TODO: LWL, LWR

(opcode #x23 LW
  (with-i-form
    `(r= ,rt (memw (+ (reg ,rs) ,(s16-ext immed))))))

(opcode #x37 LD
  (with-i-form
    `(r64= ,rt (memd (+ (reg ,rs) ,(s16-ext-immed))))))

(opcode #x24 LBU
  (with-i-form
    `(r= ,rt (memb (+ (reg ,rs) ,(s16-ext immed))))))

(opcode #x25 LHU
  (with-i-form
    `(r= ,rt (memh (+ (reg ,rs) ,(s16-ext immed))))))

; FIXME: LWU and LW implementations are the same, but should work differently
(opcode #x27 LWU
  (with-i-form
    `(r= ,rt (memw (+ (reg ,rs) ,(s16-ext immed))))))

(opcode #x28 SB
  (with-i-form
    `(mb= (+ (reg ,rs) ,(s16-ext immed)) (reg ,rt))))

(opcode #x29 SH
  (with-i-form
    `(mh= (+ (reg ,rs) ,(s16-ext immed)) (reg ,rt))))

(opcode #x2a SW
  (with-i-form
    `(mw= (+ (reg ,rs) ,(s16-ext immed)) (reg ,rt))))

(opcode #x3f SD
  (with-i-form
    `(md= (+ (reg ,rs) ,(s16-ext immed)) (reg64 ,rt))))

  ; TODO:  SWL , SDL , SDR , SWR
  ; TODO:  LL     , LWC1  , NI  , NI   , NI  , LDC1
  ; TODO:  SC     , SWC1  , NI  , NI   , NI  , SDC1

|#

  ; SPECIAL instruction handlers

  (special #x00 SLL
           (with-shift-form
            `(r= ,rd (arithmetic-shift (reg ,rt) ,sa))))

  (special #x02 SRL
           (with-shift-form
            `(r= ,rd (logical-shift (reg ,rt) ,(- sa)))))

  (special #x03 SRA
           (with-shift-form
            `(r= ,rd (arithmetic-shift (reg ,rt) ,(- sa)))))

  (special #x04 SLLV
           (with-reg-form
            `(r= ,rd (arithmetic-shift (reg ,rt) (reg ,rs)))))

  (special #x06 SRLV
           (with-reg-form
            `(r= ,rd (logical-shift (reg ,rt) (- (reg ,rs))))))

  (special #x07 SRAV
           (with-reg-form
            `(r= ,rd (arithmetic-shift (reg ,rt) (- (reg ,rs))))))

#|
(opcode #x08 JR
  (let ((rs (get-rs instr)))
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; Branch to $rs
       (branch (ureg ,rs)))))

(opcode #x09 JALR
  (let ((rs (get-rs instr))
        (rd (get-rd instr)))
    `(begin
       ; Recompile the delay slot
       ,(gen-delay)
       ; Link
       (r= ,rd ,(+ (get-pc) 4))
       ; Branch to $rs
       (branch (ureg ,rs)))))
|#

  ; TODO: SYSCALL

  (special #x10 MFHI
           (let ((rd (get-rd instr)))
             `(r= ,rd (reg "hi"))))

  (special #x11 MTHI
           (let ((rs (get-rs instr)))
             `(r= "hi" (reg ,rs))))

  (special #x12 MFLO
           (let ((rd (get-rd instr)))
             `(r= ,rd (reg "lo"))))

  (special #x13 MTLO
           (let ((rs (get-rs instr)))
             `(r= "lo" (reg ,rs))))

  (special #x14 DSLLV
           (with-reg-form
            `(r64= ,rd (arithmetic-shift (reg64 ,rt) (reg ,rs)))))

  (special #x16 DSRLV
           (with-reg-form
            `(r64= ,rd (logical-shift (reg64 ,rt) (- (reg ,rs))))))

  (special #x17 DSRAV
           (with-reg-form
            `(r64= ,rd (arithmetic-shift (reg64 ,rt) (- (reg ,rs))))))

  (special #x18 MULT
           (with-mult-form
            `(let ((product (* (reg ,rs) (reg ,rt))))
               (begin
                 (r= "lo" (bitwise-and product #xFFFFFFFF))
                 (r= "hi" (bitwise-and (arithmetic-shift product -32) #xFFFFFFFF))))))

  (special #x19 MULTU
           (with-mult-form
            `(let ((product (* (ureg ,rs) (ureg ,rt))))
               (begin
                 (r= "lo" (bitwise-and product #xFFFFFFFF))
                 (r= "hi" (bitwise-and (arithmetic-shift product -32) #xFFFFFFFF))))))

  (special #x1a DIV
           (with-mult-form
            `(begin
               (r= "lo" (quotient (reg ,rs) (reg ,rt)))
               (r= "hi" (modulo (reg ,rs) (reg ,rt))))))

  (special #x1b DIVU
           (with-mult-form
            `(begin
               (r= "lo" (quotient (ureg ,rs) (ureg ,rt)))
               (r= "hi" (modulo (ureg ,rs) (ureg ,rt))))))

  (special #x21 ADDU
           (with-reg-form
            `(r= ,rd (+ (reg ,rs) (reg ,rt)))))

  (define ADD ADDU)
  (hash-table-put! opcode-handlers #x20 ADD)

  (special #x23 SUBU
           (with-reg-form
            `(r= ,rd (- (reg ,rs) (reg ,rt)))))

  (define SUB SUBU)
  (hash-table-put! opcode-handlers #x22 SUB)

  (special #x24 AND
           (with-reg-form
            `(r= ,rd (bitwise-and (reg ,rs) (reg ,rt)))))

  (special #x25 OR
           (with-reg-form
            `(r= ,rd (bitwise-or (reg ,rs) (reg ,rt)))))

  (special #x26 XOR
           (with-reg-form
            `(r= ,rd (bitwise-xor (reg ,rs) (reg ,rt)))))

  (special #x27 NOR
           (with-reg-form
            `(r= ,rd (bitwise-nor (reg ,rs) (reg ,rt)))))

  (special #x2a SLT
           (with-reg-form
            `(r= ,rd (if (< (reg ,rs) (reg ,rt)) 1 0))))

  (special #x2b SLTU
           (with-reg-form
            `(r= ,rd (if (< (ureg ,rs) (ureg ,rt)) 1 0))))

  (special #x2d DADDU
           (with-reg-form
            `(r64= ,rd (+ (reg64 ,rs) (reg64 ,rt)))))

  (define DADD DADDU)
  (hash-table-put! opcode-handlers #x2c DADD)

  (special #x2f DSUBU
           (with-reg-form
            `(r64= ,rd (- (reg64 ,rs) (reg64 ,rt)))))

  (define DSUB DSUBU)
  (hash-table-put! opcode-handlers #x2e DSUB)

  ; TODO: TEQ if necessary

  (special #x38 DSLL
           (with-shift-form
            `(r64= ,rd (arithmetic-shift (reg64 ,rt) ,sa))))

  (special #x3a DSRL
           (with-shift-form
            `(r64= ,rd (logical-shift (reg64 ,rt) ,(- sa)))))

  (special #x3b DSRA
           (with-shift-form
            `(r64= ,rd (arithmetic-shift (reg64 ,rt) ,(- sa)))))

  (special #x3c DSLL32
           (with-shift-form
            `(r64= ,rd (arithmetic-shift (reg64 ,rt) ,(+ sa 32)))))

  (special #x3e DSRL32
           (with-shift-form
            `(r64= ,rd (logical-shift (reg64 ,rt) ,(- (+ sa 32))))))

  (special #x3f DSRA32
           (with-shift-form
            `(r64= ,rd (arithmetic-shift (reg64 ,rt) ,(- (+ sa 32))))))
  
  )


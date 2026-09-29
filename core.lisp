; Constants
(define nil ())

; Types
(define t-int (? 1))
(define t-cons (? (cons 1)))

; Log levels
(define LOG_ALLOC 0)
(define LOG_DEBUG 1)
(define LOG_INFO 2)
(define LOG_WARNING 3)
(define LOG_ERROR 4)

; Set log level
(loglevel LOG_INFO)


(define (cons? c)
                (if (eq (? c) t-cons)
                  1))

(define (reverse l)
                  "Reverse order of list"
                  (define (reverse-r old new)
                                    (if (cons? old)
                                      (reverse-r (cdr old) (cons (car old) new))
                                      new))
                  (reverse-r l ()))

(define (map func args)
              "Apply function f to elements of list l"
              (define (map-r old new)
                            (if (cons? old)
                              (map-r (cdr old) (cons (func (car old)) new))
                              new))
              (reverse (map-r args ())))

;; (exit 0)

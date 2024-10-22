(define (and a b) (if a b))
(define (or a b) (if a a b))
(define (not a) (if a nil :t))

(define /nil (? nil))
(define /int (? 0))
(define /flo (? 0.0))
(define /str (? ""))
(define /lab (? :0))
(define /sym (? 'a))
(define /pri (? ?))
(define /con (? '(0)))
(define /clo (? (lambda())))

(define (len lst)
  (if (not lst) 0
    (do (define (len-r lst l)
          (if (= lst nil)
            l
            (len-r (cdr lst) (+ l 1))))
      (len-r (cdr lst) 0))))

(define (list-eq a b)
  (if (= (len a) (len b))
    (do (define (list-eq-r a b)
          (if (= a nil) 1
            (if (= (car a) (car b))
              (list-eq-r (cdr a) (cdr b)))))
      (list-eq-r a b)))

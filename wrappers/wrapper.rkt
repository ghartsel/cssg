#lang racket/base

;; requires racket >= 5.3 because of submodules

;; Lowlevel interface

(module low-level racket/base

  (require ffi/unsafe ffi/unsafe/define)

  (provide (all-defined-out))

  (define-ffi-definer defcssg (ffi-lib "libcssg"))

  (define _cssg_node_type
    (_enum '(;; Error status
             none
             ;; Block
             document block-quote list item code-block
             html-block custom-block
             paragraph heading thematic-break
             ;; ?? first-block = document
             ;; ?? last-block = thematic-break
             ;; Inline
             text softbreak linebreak code html-inline custom-inline
             emph strong link image
             ;; ?? first-inline = text
             ;; ?? last-inline = image
             )))
  (define _cssg_list_type
    (_enum '(no_list bullet_list ordered_list)))
  (define _cssg_delim_type
    (_enum '(no_delim period_delim paren_delim)))
  (define _cssg_opts
    (let ([opts '([sourcepos  1] ; include sourcepos attribute on block elements
                  [hardbreaks 2] ; render `softbreak` elements as hard line breaks
                  [safe       3] ; defined here for API compatibility (on by default)
                  [unsafe    17] ; render raw HTML and unsafe links
                  [nobreaks   4] ; render `softbreak` elements as spaces
                  [normalize  8] ; legacy (no effect)
                  [validate-utf8 9] ; validate UTF-8 in the input
                  [smart     10] ; straight quotes to curly, ---/-- to em/en dashes
                  )])
      (_bitmask (apply append (map (λ(o) `(,(car o) = ,(expt 2 (cadr o))))
                                   opts)))))

  (define-cpointer-type _node)

  (defcssg cssg_markdown_to_html
    (_fun [bs : _bytes] [_int = (bytes-length bs)] _cssg_opts
          -> [r : _bytes] -> (begin0 (bytes->string/utf-8 r) (free r))))

  (defcssg cssg_parse_document
    (_fun [bs : _bytes] [_int = (bytes-length bs)] _cssg_opts
          -> _node))

  (defcssg cssg_render_html
    (_fun _node _cssg_opts
          -> [r : _bytes] -> (begin0 (bytes->string/utf-8 r) (free r))))

  (defcssg cssg_node_new              (_fun _cssg_node_type -> _node))
  (defcssg cssg_node_free             (_fun _node -> _void))

  (defcssg cssg_node_next             (_fun _node -> _node/null))
  (defcssg cssg_node_previous         (_fun _node -> _node/null))
  (defcssg cssg_node_parent           (_fun _node -> _node/null))
  (defcssg cssg_node_first_child      (_fun _node -> _node/null))
  (defcssg cssg_node_last_child       (_fun _node -> _node/null))

  (defcssg cssg_node_get_user_data    (_fun _node -> _racket))
  (defcssg cssg_node_set_user_data    (_fun _node _racket -> _bool))
  (defcssg cssg_node_get_type         (_fun _node -> _cssg_node_type))
  (defcssg cssg_node_get_type_string  (_fun _node -> _bytes))
  (defcssg cssg_node_get_literal      (_fun _node -> _string))
  (defcssg cssg_node_set_literal      (_fun _node _string -> _bool))
  (defcssg cssg_node_get_heading_level (_fun _node -> _int))
  (defcssg cssg_node_set_heading_level (_fun _node _int -> _bool))
  (defcssg cssg_node_get_list_type    (_fun _node -> _cssg_list_type))
  (defcssg cssg_node_set_list_type    (_fun _node _cssg_list_type -> _bool))
  (defcssg cssg_node_get_list_delim   (_fun _node -> _cssg_delim_type))
  (defcssg cssg_node_set_list_delim   (_fun _node _cssg_delim_type -> _bool))
  (defcssg cssg_node_get_list_start   (_fun _node -> _int))
  (defcssg cssg_node_set_list_start   (_fun _node _int -> _bool))
  (defcssg cssg_node_get_list_tight   (_fun _node -> _bool))
  (defcssg cssg_node_set_list_tight   (_fun _node _bool -> _bool))
  (defcssg cssg_node_get_fence_info   (_fun _node -> _string))
  (defcssg cssg_node_set_fence_info   (_fun _node _string -> _bool))
  (defcssg cssg_node_get_url          (_fun _node -> _string))
  (defcssg cssg_node_set_url          (_fun _node _string -> _bool))
  (defcssg cssg_node_get_title        (_fun _node -> _string))
  (defcssg cssg_node_set_title        (_fun _node _string -> _bool))
  (defcssg cssg_node_get_start_line   (_fun _node -> _int))
  (defcssg cssg_node_get_start_column (_fun _node -> _int))
  (defcssg cssg_node_get_end_line     (_fun _node -> _int))
  (defcssg cssg_node_get_end_column   (_fun _node -> _int))

  (defcssg cssg_node_unlink           (_fun _node -> _void))
  (defcssg cssg_node_insert_before    (_fun _node _node -> _bool))
  (defcssg cssg_node_insert_after     (_fun _node _node -> _bool))
  (defcssg cssg_node_prepend_child    (_fun _node _node -> _bool))
  (defcssg cssg_node_append_child     (_fun _node _node -> _bool))
  (defcssg cssg_consolidate_text_nodes (_fun _node -> _void))

  (defcssg cssg_version               (_fun -> _int))
  (defcssg cssg_version_string        (_fun -> _string))

  )

;; Rackety interface

(module high-level racket/base

  (require (submod ".." low-level) ffi/unsafe)

  (provide cssg-markdown-to-html)
  (define (cssg-markdown-to-html str [options '(normalize smart)])
    (cssg_markdown_to_html (if (bytes? str) str (string->bytes/utf-8 str))
                            options))

  (require (for-syntax racket/base racket/syntax))
  (define-syntax (make-getter+setter stx)
    (syntax-case stx ()
      [(_ name) (with-syntax ([(getter setter)
                               (map (λ(op) (format-id #'name "cssg_node_~a_~a"
                                                      op #'name))
                                    '(get set))])
                  #'(cons getter setter))]))
  (define-syntax-rule (define-getters+setters name [type field ...] ...)
    (define name (list (list 'type (make-getter+setter field) ...) ...)))
  (define-getters+setters getters+setters
    [heading heading_level] [code-block fence_info]
    [link url title] [image url title]
    [list list_type list_delim list_start list_tight])

  (provide cssg->sexpr)
  (define (cssg->sexpr node)
    (define text (cssg_node_get_literal node))
    (define type (cssg_node_get_type node))
    (define children
      (let loop ([node (cssg_node_first_child node)])
        (if (not node) '()
            (cons (cssg->sexpr node) (loop (cssg_node_next node))))))
    (define info
      (cond [(assq type getters+setters)
             => (λ(gss) (map (λ(gs) ((car gs) node)) (cdr gss)))]
            [else '()]))
    (define (assert-no what-not b)
      (when b (error 'cssg->sexpr "unexpected ~a in ~s" what-not type)))
    (cond [(memq type '(document paragraph heading block-quote list item
                        emph strong link image))
           (assert-no 'text text)
           (list type info children)]
          [(memq type '(text code code-block html-block html-inline
                        softbreak linebreak thematic-break))
           (assert-no 'children (pair? children))
           (list type info text)]
          [else (error 'cssg->sexpr "unknown type: ~s" type)]))

  (provide sexpr->cssg)
  (define (sexpr->cssg sexpr) ; assumes valid input, as generated by the above
    (define (loop sexpr)
      (define type (car sexpr))
      (define info (cadr sexpr))
      (define data (caddr sexpr))
      (define node (cssg_node_new type))
      (let ([gss (assq type getters+setters)])
        (when gss
          (unless (= (length (cdr gss)) (length info))
            (error 'sexpr->cssg "bad number of info values in ~s" sexpr))
          (for-each (λ(gs x) ((cdr gs) node x)) (cdr gss) info)))
      (cond [(string? data) (cssg_node_set_literal node data)]
            [(not data) (void)]
            [(list? data)
             (for ([child (in-list data)])
               (cssg_node_append_child node (sexpr->cssg child)))]
            [else (error 'sexpr->cssg "bad data in ~s" sexpr)])
      node)
    (define root (loop sexpr))
    (register-finalizer root cssg_node_free)
    root)

  ;; Registers a `cssg_node_free` finalizer
  (provide cssg-parse-document)
  (define (cssg-parse-document str [options '(normalize smart)])
    (define root (cssg_parse_document
                  (if (bytes? str) str (string->bytes/utf-8 str))
                  options))
    (register-finalizer root cssg_node_free)
    root)

  (provide cssg-render-html)
  (define (cssg-render-html root [options '(normalize smart)])
    (cssg_render_html root options)))

#; ;; sample use
(begin
  (require 'high-level racket/string)
  (cssg-render-html
   (cssg-parse-document
    (string-join '("foo"
                   "==="
                   ""
                   "> blah"
                   ">"
                   "> blah *blah* `bar()` blah:"
                   ">"
                   ">     function foo() {"
                   ">       bar();"
                   ">     }")
                 "\n"))))

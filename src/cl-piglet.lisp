;;; -*- package: CFFI; Syntax: Common-lisp; Base: 10 -*-

;;;
;;;;  cl-piglet.lisp
;;;
;;;   Author: Ora Lassila mailto:ora.lassila@nokia.com
;;;   Copyright (c) 2001-2008 Nokia. All Rights Reserved.
;;;


(in-package "CFFI")


;;; --------------------------------------------------------------------------------------
;;;
;;;   PRELIMINARIES
;;;

(eval-when (:compile-toplevel :load-toplevel :execute)
  ;; Make sure cffi:*foreign-library-directories* is set appropriately
  (dolist (library '("libcurl.dylib"
		     "libraptor.dylib"
		     "libsqlite3.dylib"
		     "libpiglet.dylib"))
    (load-foreign-library library)))

(eval-when (:compile-toplevel :load-toplevel :execute)
  (defpackage "PIGLET"
    (:use "CL")
    (:export "%OPEN"
	     "%CLOSE"
	     "%QUERY"
	     "%QUERY-COLLECTED"
	     "%SOURCES"
	     "%SOURCES-COLLECTED"
	     "%ADD"
	     "%DEL"
	     "%DEL-SOURCE"
	     "%LOAD"
	     "%INFO"
	     "%NODE"
	     "%LITERAL"
	     "%COUNT"
	     "%NODE-TO-STRING"
	     "%TRIPLE-TO-STRING"
	     "%EXPAND"
	     "%ABBREVIATE"
	     "%ADD-NAMESPACE"
	     "%DEL-NAMESPACE"
	     "DEFCFUN"
	     "*COLLECTED-RESULTS*"
	     "*TRIPLE-MAKER*"
	     "*NODE-MAKER*"
	     "TRIPLE-COLLECTOR"
	     "NODE-COLLECTOR")))

(eval-when (:compile-toplevel :load-toplevel :execute)
  (defmacro piglet:defcfun ((c-name cl-name) return-type &rest params)
    `(progn
       (declaim (inline ,cl-name))
       (defcfun (,c-name ,cl-name) ,return-type ,@params))))


;;; --------------------------------------------------------------------------------------
;;;
;;;   LIBRARY ENTRY POINTS
;;;

(piglet:defcfun ("piglet_open" piglet:%open) :pointer
  (name :string))

(piglet:defcfun ("piglet_close" piglet:%close) :boolean
  (db :pointer))

(piglet:defcfun ("piglet_query" piglet:%query) :boolean
  (db :pointer)
  (s :int)
  (p :int)
  (o :int)
  (userdata :pointer)
  (callback :pointer))

(piglet:defcfun ("piglet_sources" piglet:%sources) :boolean
  (db :pointer)
  (s :int)
  (p :int)
  (o :int)
  (userdata :pointer)
  (callback :pointer))

(piglet:defcfun ("piglet_add" piglet:%add) :boolean
  (db :pointer)
  (s :int)
  (p :int)
  (o :int)
  (source :int)
  (temporary :short))

(piglet:defcfun ("piglet_del" piglet:%del) :boolean
  (db :pointer)
  (s :int)
  (p :int)
  (o :int)
  (source :int)
  (temporary :short))

(piglet:defcfun ("piglet_del_source" piglet:%del-source) :boolean
  (db :pointer)
  (source :int)
  (triplesonly :short))

(piglet:defcfun ("piglet_load" piglet::%load-inner) :boolean
  (db :pointer)
  (source :int)
  (append :short)
  (verbose :short)
  (script :string)
  (args :pointer))

(piglet:defcfun ("piglet_info" piglet::%info-inner) :pointer
  (db :pointer)
  (node :int)
  (datatype :pointer)
  (lang :pointer))

(piglet:defcfun ("piglet_node" piglet:%node) :int
  (db :pointer)
  (uri :string))

(piglet:defcfun ("piglet_literal" piglet:%literal) :int
  (db :pointer)
  (string :string)
  (datatype :int)
  (lang :string))

(piglet:defcfun ("piglet_count" piglet:%count) :int
  (db :pointer))

(piglet:defcfun ("piglet_node_tostring" piglet:%node-to-string) :string
  (db :pointer)
  (node :int))

(piglet:defcfun ("piglet_triple_tostring" piglet:%triple-to-string) :string
  (db :pointer)
  (s :int)
  (p :int)
  (o :int))

(piglet:defcfun ("piglet_expand" piglet:%expand) :string
  (db :pointer)
  (qname :string))

(piglet:defcfun ("piglet_abbreviate" piglet:%abbreviate) :string
  (db :pointer)
  (uri :string))

(piglet:defcfun ("piglet_add_namespace" piglet:%add-namespace) :boolean
  (db :pointer)
  (prefix :string)
  (uri :string))

(piglet:defcfun ("piglet_del_namespace" piglet:%del-namespace) :boolean
  (db :pointer)
  (prefix :string))


;;; --------------------------------------------------------------------------------------
;;;
;;;   ADDITIONAL FUNCTIONS
;;;

(defun piglet:%load (db source append verbose script &rest args)
  (declare (dynamic-extent args))
  (assert (< (length args) 10))
  (let* ((argv (foreign-alloc :string :initial-contents args :null-terminated-p t))
	 (result (piglet::%load-inner db source append verbose script argv)))
    (dotimes (i (length args))
      (foreign-free (mem-aref argv :pointer i)))
    (foreign-free argv)
    result))
    
(defun piglet:%info (db node)
  (if (> node 0)
    (let ((u (piglet::%info-inner db node (null-pointer) (null-pointer))))
      (prog1 (foreign-string-to-lisp u)
	(foreign-free u)))
    (with-foreign-pointer (dt 4)
      (with-foreign-pointer (lang 64)
	(let ((s (piglet::%info-inner db node dt lang)))
	  (let ((str (foreign-string-to-lisp s)))
	    (foreign-free s)
	    (values str
		    (mem-ref dt :int 0)
		    (foreign-string-to-lisp lang))))))))

;; We rely on the fact that separate threads get their own individual special bindings
(defvar piglet:*collected-results*)

(defvar piglet:*triple-maker* #'(lambda (s p o) (list s p o)))

(defvar piglet:*node-maker* #'identity)

(defcallback piglet:triple-collector :short ((db :pointer)
					     (userdata :pointer)
					     (s :int)
					     (p :int)
					     (o :int))
  (declare (ignore db userdata))
  (push (funcall piglet:*triple-maker* s p o) piglet:*collected-results*)
  1)

(defcallback piglet:node-collector :short ((db :pointer)
					   (userdata :pointer)
					   (node :int))
  (declare (ignore db userdata))
  (push (funcall piglet:*node-maker* node) piglet:*collected-results*)
  1)

(defun piglet:%query-collected (db s p o
				&optional (triple-maker piglet:*triple-maker*))
  (let ((piglet:*collected-results* nil)
	(piglet:*triple-maker* triple-maker))
    (when (piglet:%query db s p o
			 (null-pointer)
			 (callback piglet:triple-collector))
      (nreverse piglet:*collected-results*))))

(defun piglet:%sources-collected (db s p o
					 &optional (node-maker piglet:*node-maker*))
  (let ((piglet:*collected-results* nil)
	(piglet:*node-maker* node-maker))
    (when (piglet:%sources db s p o
			   (null-pointer)
			   (callback piglet:node-collector))
      (nreverse piglet:*collected-results*))))

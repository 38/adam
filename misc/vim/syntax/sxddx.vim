if exists("b:current_syntax")
	finish
endif
syn keyword sxddxKeyword move return const monitor check instance array new filled 16 from16 wide object result exception void high16 jumbo enter exit cast of length range throw goto packed switch sparse cmpl cmpg cmp float double long if eq ne le ge gt lt eqz nez lez gez gtz ltz boolean byte char short aget aput sget sput iget iput invoke virtual super direct static int to neg not add sub mul div rem and or xor shl shr ushl ushr 2addr lit8 lit16 nop string default rsub attrs line limit registers label source implements data catch catchall fill using from
syn keyword defination class method field interface
syn keyword sxddxAttribute final private protected public synchronized transient abstract annotation
syn region sxStorageClass start=/(\s*attrs/ end=/)/ contains=sxddxAttribute
syn match ClassPath /[a-zA-Z_][a-zA-Z0-9_\/]*[a-zA-Z0-9_]/
syn match Number "\<\(0[0-7]*\|0[xX]\x\+\|\d\+\)[lL]\=\>"
syn match Number "\(\<\d\+\.\d*\|\.\d\+\)\([eE][-+]\=\d\+\)\=[fFdD]\="
syn match Number "\<\d\+[eE][-+]\=\d\+[fFdD]\=\>"
syn match Number "\<\d\+\([eE][-+]\=\d\+\)\=[fFdD]\>"
syn region sxComment start=/;/ end=/$/
syn region sxString start="\"[^"]" skip="\\\"" end="\"" contains=scalaStringEscape " TODO end \n or not?
syn match sxStringEscape "\\u[0-9a-fA-F]\{4}" contained
syn match sxStringEscape "\\[nrfvb\\\"]" contained

hi link sxddxKeyword Keyword
hi link sxNumber Number 
hi link sxString String
hi link sxStorageClass StorageClass
hi link classpath String
hi link defination Function
hi link ClassPath String
hi link sxComment Comment

let b:current_syntax = "sxddx"

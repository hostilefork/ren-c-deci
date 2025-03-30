REBOL []

name: 'Deci
source: %deci/mod-deci.c
includes: [
    %prep/extensions/deci
]
depends: [
    [
        %deci/deci.c

        ; May 2018 update to MSVC 2017 added warnings for Spectre mitigation.
        ; %deci.c is a lot of twiddly custom C code for implementing a fixed
        ; precision math type, that was for some reason a priority in R3-Alpha
        ; but isn't very central to Ren-C.  It is not a priority to audit
        ; it for speed, so allow it to be slow if MSVC compiles with /Qspectre
        ;
        <msc:/wd5045>  ; https://stackoverflow.com/q/50399940
    ]
]

use-librebol: 'no

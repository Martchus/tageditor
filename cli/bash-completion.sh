__tageditorcompappend ()
{       
    local x i=${#COMPREPLY[@]}
    for x in $1; do
        if [[ "$x" == "$3"* ]]; then
                COMPREPLY[i++]="$2$x$4"
        fi
    done
}


__tageditorcomp ()
{
    local cur_="${3-$cur}"
    case "$cur_" in
    --*=)
        ;;
    *)
        local c i=0 IFS=$' \t\n'
        for c in $1; do
            c="$c${4-}"
            if [[ $c == "$cur_"* ]]; then
                case $c in
                --*=*|*.) ;;
                *) c="$c " ;;
                esac
                COMPREPLY[i++]="${2-}$c"
            fi
        done
        ;;
    esac
}

__tageditorcomp_nl_append ()
{
    local IFS=$'\n'
    __tageditorcompappend "$1" "${2-}" "${3-$cur}" "${4- }"
}


__tageditorcomp_nl ()
{
    COMPREPLY=()
    __tageditorcomp_nl_append "$@"
}


_tageditor () 
{
    local cur prev opts base
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    #
    #  The basic options we'll complete.
    #
    opts="get set --help"


    #
    #  Complete the arguments to some of the basic commands.
    #
    case "${cur}" in
        
    esac
    
    case "${prev}" in
        =)
            COMPREPLY=( $(compgen -A file -- ${cur}) )
            return 0
            ;;
        cover)
            COMPREPLY=( $(compgen -A file) )
            return 0
            ;;
        get)
            #__tageditorcomp $(tageditor --print-field-names)
            COMPREPLY=( $(compgen -W "$(tageditor --print-field-names)" -- ${cur}) )
            return 0
            ;;
        set)
            #__tageditorcomp_nl "$(tageditor --print-field-names)"
            COMPREPLY=( $(compgen -W "$(tageditor --print-field-names)" -S = -- ${cur}) )
            return 0
            ;;
        *)
        ;;
    esac

   COMPREPLY=($(compgen -W "${opts}" -- ${cur}))  
   return 0
}
complete -F _tageditor tageditor

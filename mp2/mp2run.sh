#!/usr/bin/env bash
# vim: ft=sh ts=4 sw=4 sts=4 et :


optargerr() {
    echo "ERROR: invalid argument for opt \"$1\"." >&2
    exit 1
}

phelp() {
    echo "Usage: $0 [OPTION]... [sender|receiver]"
    echo "OPTION: "
    echo "  --receiverip|-r <ip> IP address of the receiver."
    echo "  --port|-p <port>     Port number to use."
    echo "  --filesent|-f <file> File to send."
    echo "  --newfile|-F <file>  New file to receive."
    echo "  --bytes|-n <bytes>   Number of bytes to send."
    echo "  --debug|-g           Run in debug mode."
    echo "  --break|-b           Automatically add break points in debug mode."
}

main() {
    if [[ -z "$1" ]]; then
        phelp
        exit 1
    fi

    local sender receiver receiverip port filesent newfile bytes dbg break exe;
    sender='./reliable_sender'
    receiver='./reliable_receiver'
    receiverip='192.168.56.101'
    port='2223'
    filesent='Makefile'
    newfile='NewFile'
    bytes='100000'
    dbg='false'
    break='false'
    exe=''

    while :; do
        case $1 in
            --help|-h)
                phelp
                exit 0
                ;;
            --receiverip|-r)
                if [[ -n "$2" ]]; then
                    receiverip="$2"
                    shift 2
                else
                    optargerr "$1"
                fi
                ;;
            --port|-p)
                if [[ -n "$2" ]]; then
                    port="$2"
                    shift 2
                else
                    optargerr "$1"
                fi
                ;;
            --filesent|-f)
                if [[ -n "$2" && -f "$2" ]]; then
                    filesent="$2"
                    shift 2
                else
                    optargerr "$1"
                fi
                ;;
            --newfile|-F)
                if [[ -n "$2" ]]; then
                    newfile="$2"
                    shift 2
                else
                    optargerr "$1"
                fi
                ;;
            --bytes|-n)
                if [[ -n "$2" ]]; then
                    bytes="$2"
                    shift 2
                else
                    optargerr "$1"
                fi
                ;;
            --debug|-g)
                dbg='true'
                shift
                ;;
            --break|-b)
                break='true'
                shift
                ;;
            sender)
                exe="$sender"
                shift
                ;;
            receiver)
                exe="$receiver"
                shift
                ;;
            *)
                break
                ;;
        esac
    done

    if [[ -z "$exe" ]]; then
        echo "ERROR: no executable specified." >&2
        exit 1
    else
        if [[ "$exe" = "$sender" ]]; then
            if [[ "$dbg" = true ]]; then
                if [[ "$break" = true ]]; then
                    gdb -ex 'b rdt_sender_state_in'            \
                        -ex 'b rdt_sender_state_ss'            \
                        -ex 'b rdt_sender_state_ca'            \
                        -ex 'b rdt_sender_state_fr'            \
                        --args "$sender" "$receiverip" "$port" \
                               "$filesent" "$bytes"
                else
                    gdb --args "$sender" "$receiverip" "$port" \
                               "$filesent" "$bytes"
                fi
            else
                "$sender" "$receiverip" "$port" "$filesent" "$bytes"
            fi
        else
            if [[ "$dbg" = true ]]; then
                if [[ "$break" = true ]]; then
                    gdb -ex 'b reliablyReceive' \
                        --args "$receiver" "$port" "$newfile"
                else
                    gdb --args "$receiver" "$port" "$newfile"
                fi
            else
                "$receiver" "$port" "$newfile"
            fi
        fi
    fi
}

main "$@"

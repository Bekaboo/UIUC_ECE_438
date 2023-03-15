#!/usr/bin/env bash
# vim: ft=sh ts=4 sw=4 sts=4 et :

main() {
    if [[ -z "$1" ]]; then
        echo "Usage: $0 [sender|receiver|dbg-sender]" >&2
        exit 1
    else
        local sender receiver receiverip port filesent filewritten bytes;
        sender='./reliable_sender'
        receiver='./reliable_receiver'
        receiverip='192.168.56.101'
        port='2223'
        filesent='Makefile'
        filewritten='newfile'
        bytes='10000'

        while :; do
            case $1 in
                sender)
                    "$sender" "$receiverip" "$port" "$filesent" "$bytes"
                    break;
                    ;;
                receiver)
                    "$receiver" "$port" "$filewritten"
                    break;
                    ;;
                dbg-sender)
                    gdb -ex 'b rdt_sender_state_in' \
                        -ex 'b rdt_sender_state_ss' \
                        -ex 'b rdt_sender_state_ca' \
                        -ex 'b rdt_sender_state_fr' \
                        --args "$sender" "$receiverip" "$port" "$filesent" "$bytes"
                    break;
                    ;;
                *)
                    echo "Usage: $0 [sender|receiver|dbg-sender]" >&2
                    exit 1;
                    ;;
            esac
            shift
        done
        fi
}

main "$1"

#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
BOLD='\033[1m'
RESET='\033[0m'

# Header
echo -e "${BOLD}${CYAN}"
echo "╔══════════════════════════════════════════════════════════╗"
echo "║              TODO LIST REDIS DATABASE VIEWER             ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo -e "${RESET}"

# Database stats
echo -e "\n${BOLD}${CYAN}📊 DATABASE INFO${RESET}"
echo -e "${CYAN}──────────────────────────────────────────────────────────${RESET}"
echo -e "${GREEN}●${RESET} ${WHITE}Redis Keys:${RESET}     $(redis-cli DBSIZE 2>/dev/null)"
echo -e "${GREEN}●${RESET} ${WHITE}Redis Version:${RESET} $(redis-cli INFO server | grep redis_version | cut -d: -f2)"
echo -e "${GREEN}●${RESET} ${WHITE}Memory Used:${RESET}  $(redis-cli INFO memory | grep used_memory_human | cut -d: -f2)"

# Counter value
echo -e "\n${BOLD}${CYAN}🔢 SEQUENCE COUNTER${RESET}"
echo -e "${CYAN}──────────────────────────────────────────────────────────${RESET}"
counter=$(redis-cli GET todo_counter 2>/dev/null)
echo -e "${GREEN}●${RESET} ${WHITE}Next Task ID:${RESET} ${YELLOW}$counter${RESET}"

# All tasks
echo -e "\n${BOLD}${CYAN}📋 ALL TASKS${RESET}"
echo -e "${CYAN}──────────────────────────────────────────────────────────${RESET}"

redis-cli HGETALL todos 2>/dev/null | paste - - | while read id json; do
    title=$(echo "$json" | jq -r '.title' 2>/dev/null)
    priority=$(echo "$json" | jq -r '.priority' 2>/dev/null)
    status=$(echo "$json" | jq -r '.status' 2>/dev/null)
    due=$(echo "$json" | jq -r '.due_date' 2>/dev/null)
    created=$(echo "$json" | jq -r '.created_at' 2>/dev/null)
    completed=$(echo "$json" | jq -r '.completed_at' 2>/dev/null)
    
    # Priority color
    pcolor=$YELLOW
    [ "$priority" = "critical" ] && pcolor=$RED
    [ "$priority" = "high" ] && pcolor=$RED
    [ "$priority" = "low" ] && pcolor=$GREEN
    
    # Status icon
    if [ "$status" = "completed" ]; then
        icon="${GREEN}✅${RESET}"
        status_color=$GREEN
    else
        icon="${RED}⏳${RESET}"
        status_color=$RED
    fi
    
    echo ""
    echo -e "${BOLD}${CYAN}┌────────────────────────────────────────────────────────┐${RESET}"
    echo -e "${BOLD}│ Task #${WHITE}$id${RESET}${BOLD}${RESET}"
    echo -e "${CYAN}├────────────────────────────────────────────────────────┤${RESET}"
    echo -e "│ ${WHITE}Title:${RESET}     ${title}"
    echo -e "│ ${WHITE}Priority:${RESET}   ${pcolor}$priority${RESET}"
    echo -e "│ ${WHITE}Status:${RESET}     ${status_color}$icon $status${RESET}"
    echo -e "│ ${WHITE}Due Date:${RESET}   ${BLUE}$due${RESET}"
    echo -e "│ ${WHITE}Created:${RESET}    ${BLUE}$created${RESET}"
    [ ! -z "$completed" ] && [ "$completed" != "null" ] && \
        echo -e "│ ${WHITE}Completed:${RESET} ${GREEN}$completed${RESET}"
    echo -e "${CYAN}└────────────────────────────────────────────────────────┘${RESET}"
done

# If no tasks
[ -z "$(redis-cli HKEYS todos 2>/dev/null)" ] && echo -e "${YELLOW}  No tasks found${RESET}"

echo ""
echo -e "${CYAN}════════════════════════════════════════════════════════════${RESET}"
echo -e "${DIM}Redis DB path: $(redis-cli CONFIG GET dir | tail -1)/dump.rdb${RESET}"
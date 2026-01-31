#!/bin/bash

# BakkesMod Hooks Validation Script
# Tests that all hooks are properly configured

# Don't exit on error - we want to count all failures
# set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "🔍 Validating BakkesMod Hooks Configuration..."
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
PASSED=0
FAILED=0

# Test function
test_file() {
    local file="$1"
    local desc="$2"
    
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $desc"
        ((PASSED++))
    else
        echo -e "${RED}✗${NC} $desc - MISSING: $file"
        ((FAILED++))
    fi
}

test_dir() {
    local dir="$1"
    local desc="$2"
    
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC} $desc"
        ((PASSED++))
    else
        echo -e "${RED}✗${NC} $desc - MISSING: $dir"
        ((FAILED++))
    fi
}

# Test hook directory
echo "📁 Testing Hook Directory Structure..."
test_dir "$PROJECT_ROOT/.claude/bakkesmod-hooks" "Hook directory exists"

# Test hook files
echo ""
echo "📄 Testing Hook Documentation Files..."
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/README.md" "README.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/pre-build.md" "pre-build.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/post-build.md" "post-build.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/pre-edit.md" "pre-edit.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/post-edit.md" "post-edit.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/project-init.md" "project-init.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/cvar-validation.md" "cvar-validation.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/IMPLEMENTATION_SUMMARY.md" "IMPLEMENTATION_SUMMARY.md"
test_file "$PROJECT_ROOT/.claude/bakkesmod-hooks/QUICK_REFERENCE.md" "QUICK_REFERENCE.md"

# Test context file
echo ""
echo "📖 Testing Context Documentation..."
test_file "$PROJECT_ROOT/.claude/bakkesmod-context.md" "bakkesmod-context.md"

# Test configuration
echo ""
echo "⚙️  Testing Configuration..."
test_file "$PROJECT_ROOT/.claude-flow/config.yaml" "config.yaml"

# Validate config.yaml contains BakkesMod settings
if [ -f "$PROJECT_ROOT/.claude-flow/config.yaml" ]; then
    if grep -q "bakkesmod-plugin" "$PROJECT_ROOT/.claude-flow/config.yaml"; then
        echo -e "${GREEN}✓${NC} config.yaml contains BakkesMod project type"
        ((PASSED++))
    else
        echo -e "${RED}✗${NC} config.yaml missing BakkesMod project type"
        ((FAILED++))
    fi
    
    if grep -q "customPaths:" "$PROJECT_ROOT/.claude-flow/config.yaml"; then
        echo -e "${GREEN}✓${NC} config.yaml contains custom paths"
        ((PASSED++))
    else
        echo -e "${RED}✗${NC} config.yaml missing custom paths"
        ((FAILED++))
    fi
fi

# Test file sizes (ensure not empty)
echo ""
echo "📊 Testing File Sizes..."

check_file_size() {
    local file="$1"
    local min_size="$2"
    local desc="$3"
    
    if [ -f "$file" ]; then
        local size=$(wc -c < "$file")
        if [ $size -gt $min_size ]; then
            echo -e "${GREEN}✓${NC} $desc (${size} bytes)"
            ((PASSED++))
        else
            echo -e "${RED}✗${NC} $desc - Too small: ${size} bytes (min: ${min_size})"
            ((FAILED++))
        fi
    fi
}

check_file_size "$PROJECT_ROOT/.claude/bakkesmod-hooks/README.md" 3000 "README.md size"
check_file_size "$PROJECT_ROOT/.claude/bakkesmod-hooks/pre-edit.md" 6000 "pre-edit.md size"
check_file_size "$PROJECT_ROOT/.claude/bakkesmod-hooks/post-edit.md" 8000 "post-edit.md size"
check_file_size "$PROJECT_ROOT/.claude/bakkesmod-hooks/cvar-validation.md" 10000 "cvar-validation.md size"
check_file_size "$PROJECT_ROOT/.claude/bakkesmod-context.md" 11000 "bakkesmod-context.md size"

# Test content validation
echo ""
echo "🔎 Testing Content Validation..."

check_content() {
    local file="$1"
    local pattern="$2"
    local desc="$3"
    
    if [ -f "$file" ] && grep -q "$pattern" "$file"; then
        echo -e "${GREEN}✓${NC} $desc"
        ((PASSED++))
    else
        echo -e "${RED}✗${NC} $desc - Pattern not found: $pattern"
        ((FAILED++))
    fi
}

check_content "$PROJECT_ROOT/.claude/bakkesmod-hooks/pre-edit.md" "ImGui v1.75" "pre-edit.md mentions ImGui v1.75"
check_content "$PROJECT_ROOT/.claude/bakkesmod-hooks/pre-edit.md" "suitespot_" "pre-edit.md mentions CVar prefix"
check_content "$PROJECT_ROOT/.claude/bakkesmod-hooks/post-edit.md" "thread safety" "post-edit.md mentions thread safety"
check_content "$PROJECT_ROOT/.claude/bakkesmod-hooks/cvar-validation.md" "suitespot_\[a-z\]" "cvar-validation.md has naming regex"
check_content "$PROJECT_ROOT/.claude/bakkesmod-context.md" "SetTimeout" "bakkesmod-context.md mentions SetTimeout"
check_content "$PROJECT_ROOT/.claude-flow/config.yaml" "cpp20" "config.yaml specifies C++20"

# Summary
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 Validation Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}Passed:${NC} $PASSED"
echo -e "${RED}Failed:${NC} $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All validations passed!${NC}"
    echo ""
    echo "🎉 BakkesMod hooks are properly configured!"
    echo ""
    echo "Next steps:"
    echo "  1. Review .claude/bakkesmod-hooks/QUICK_REFERENCE.md"
    echo "  2. Run: npx claude-flow hook project-init --project bakkesmod"
    echo "  3. Start developing with automatic hook execution"
    exit 0
else
    echo -e "${RED}❌ Some validations failed!${NC}"
    echo ""
    echo "Please check the errors above and ensure all hook files are present."
    exit 1
fi

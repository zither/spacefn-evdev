#!/bin/bash

set -e

echo "=== spacefn-evdev 安装脚本 ==="
echo

RULES_FILE="rules/99-spacefn.rules"
CONFIG_DIR="$HOME/.config/spacefn"

if [ "$EUID" -ne 0 ]; then
    echo "错误: 此脚本需要 root 权限"
    echo "请使用 sudo 运行"
    exit 1
fi

echo "[1/3] 安装 udev 规则..."
if [ -f "/etc/udev/rules.d/99-spacefn.rules" ]; then
    echo "  udev 规则已存在，跳过"
else
    if [ -f "$RULES_FILE" ]; then
        cp "$RULES_FILE" /etc/udev/rules.d/
        chmod 644 /etc/udev/rules.d/99-spacefn.rules
        udevadm control --reload-rules
        echo "  已安装 udev 规则"
    else
        echo "  错误: 找不到 $RULES_FILE"
        exit 1
    fi
fi

echo
echo "[2/3] 添加用户到 input 组..."
TARGET_USER=${SUDO_USER:-$(whoami)}
if groups "$TARGET_USER" | grep -qw input; then
    echo "  用户 $TARGET_USER 已在 input 组中"
else
    usermod -aG input "$TARGET_USER"
    echo "  已将 $TARGET_USER 添加到 input 组"
    echo "  注意: 请重新登录以使组权限生效"
fi

echo
echo "[3/3] 创建配置文件目录..."
mkdir -p "$CONFIG_DIR"
if [ -d "configs" ]; then
    cp configs/*.cfg_example "$CONFIG_DIR/" 2>/dev/null || true
    echo "  已复制配置示例到 $CONFIG_DIR/"
fi
echo

echo "=== 安装完成 ==="
echo
echo "使用方式:"
echo "  ./build.sh           # 编译程序"
echo "  ./spacefn \$HOME/.config/spacefn/default.cfg_example  # 运行"
echo
echo "请重新登录后使用"

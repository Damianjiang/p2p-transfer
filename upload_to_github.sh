#!/bin/bash

# P2P Transfer - 上传到 GitHub
# 使用方法: ./upload_to_github.sh

REPO_NAME="p2p-transfer"
DESCRIPTION="Direct peer-to-peer file transfer without server"

echo "=================================="
echo "  P2P Transfer - GitHub 上传工具"
echo "=================================="
echo ""

# 检查 git 是否安装
if ! command -v git &> /dev/null; then
    echo "❌ Git 未安装"
    exit 1
fi

# 检查是否在 git 仓库中
if [ ! -d .git ]; then
    echo "❌ 当前目录不是 Git 仓库"
    echo "请在 p2p_transfer 目录中运行此脚本"
    exit 1
fi

echo "✓ Git 已安装"
echo ""

# 检查远程仓库是否已配置
if git remote get-url origin &> /dev/null; then
    echo "⚠️  远程仓库已配置:"
    git remote -v
    echo ""
    read -p "是否要更新远程仓库? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "取消操作"
        exit 0
    fi
else
    # 添加远程仓库
    echo "请选择上传方式:"
    echo "1. 使用 GitHub Token (需要从 github.com/settings/tokens 生成)"
    echo "2. 使用 SSH Key (需要先配置 SSH Key)"
    echo "3. 我已经有远程地址"
    read -p "请选择 (1/2/3): " choice
    
    case $choice in
        1)
            echo ""
            echo "请从以下网址生成 Personal Access Token:"
            echo "https://github.com/settings/tokens"
            echo "需要勾选 'repo' 权限"
            read -p "请输入 Token: " token
            read -p "请输入 GitHub 用户名: " username
            
            if [ -z "$token" ] || [ -z "$username" ]; then
                echo "❌ Token 或用户名不能为空"
                exit 1
            fi
            
            remote_url="https://${token}@github.com/${username}/${REPO_NAME}.git"
            ;;
        2)
            echo ""
            echo "请确保已配置 SSH Key"
            echo "查看: https://github.com/settings/keys"
            read -p "请输入 GitHub 用户名: " username
            remote_url="git@github.com:${username}/${REPO_NAME}.git"
            ;;
        3)
            echo ""
            echo "请输入远程仓库地址 (例如: https://github.com/用户名/仓库名.git):"
            read remote_url
            ;;
        *)
            echo "❌ 无效选择"
            exit 1
            ;;
    esac
    
    echo ""
    echo "添加远程仓库..."
    git remote add origin "$remote_url"
fi

# 创建 GitHub 仓库 (使用 API)
if [ "$choice" = "1" ] && [ -n "$token" ] && [ -n "$username" ]; then
    echo ""
    echo "尝试创建 GitHub 仓库..."
    
    response=$(curl -s -X POST \
        -H "Authorization: token ${token}" \
        -H "Content-Type: application/json" \
        -d "{\"name\":\"${REPO_NAME}\",\"description\":\"${DESCRIPTION}\",\"private\":false}" \
        https://api.github.com/user/repos 2>&1)
    
    if echo "$response" | grep -q '"id"'; then
        echo "✓ GitHub 仓库创建成功"
    elif echo "$response" | grep -q '"Already exists"'; then
        echo "⚠️  仓库已存在，跳过创建"
    else
        echo "⚠️  仓库创建失败或已存在，继续推送..."
        echo "$response" | head -5
    fi
fi

# 配置 git 用户 (如果需要)
if [ -z "$(git config user.email)" ]; then
    echo ""
    echo "配置 Git 用户信息:"
    read -p "邮箱: " email
    read -p "用户名: " name
    git config user.email "$email"
    git config user.name "$name"
fi

# 添加所有文件
echo ""
echo "添加文件到 Git..."
git add .

# 提交
echo ""
echo "提交更改:"
read -p "提交信息 (留空使用默认): " commit_msg
if [ -z "$commit_msg" ]; then
    commit_msg="Initial commit: P2P file transfer application"
fi
git commit -m "$commit_msg"

# 推送
echo ""
echo "推送到 GitHub..."
echo "注意: 如果是首次推送 master 分支，可能需要设置上游分支"
echo ""

if git push -u origin master; then
    echo ""
    echo "=================================="
    echo "  ✓ 成功！"
    echo "=================================="
    echo "仓库地址: https://github.com/$(echo $remote_url | grep -oP '(?<=github\.com[/:])[^/]+/[^/]+' | sed 's/\.git$//')"
    echo ""
    echo "下一步:"
    echo "1. 访问上面的仓库地址"
    echo "2. 添加 README.md 和其他文档"
    echo "3. 邀请 collaborators (如果有)"
    echo "4. 开启 Issues 进行问题跟踪"
else
    echo ""
    echo "❌ 推送失败"
    echo ""
    echo "常见问题:"
    echo "1. Token 权限不足 - 确保勾选了 repo 权限"
    echo "2. 仓库名已存在 - 换一个名字或删除现有仓库"
    echo "3. SSH Key 未授权 - 检查 ~/.ssh/ 配置"
    echo ""
    echo "如果已手动创建仓库，可以手动添加远程:"
    echo "  git remote set-url origin <你的仓库地址>"
    echo "  git push -u origin master"
fi

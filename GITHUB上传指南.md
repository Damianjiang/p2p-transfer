# 上传到 GitHub 指南

## 方法一：使用上传脚本（推荐）

### 1. 在本地环境运行

首先，你需要将项目下载到本地：

```bash
# 如果你已经在本地有这个项目，直接进入项目目录
cd p2p_transfer

# 或者克隆这个仓库（等我们上传后）
git clone https://github.com/你的用户名/p2p-transfer.git
cd p2p-transfer
```

### 2. 配置 GitHub 认证

#### 方式 A：使用 Personal Access Token（推荐）

1. 访问 https://github.com/settings/tokens
2. 点击 "Generate new token (classic)"
3. 勾选 `repo` 权限
4. 生成 Token 并保存
5. 运行上传脚本：
```bash
chmod +x upload_to_github.sh
./upload_to_github.sh
```
6. 选择选项 1，输入 Token 和用户名

#### 方式 B：使用 SSH Key

1. 生成 SSH Key（如果没有）：
```bash
ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
```

2. 将公钥添加到 GitHub：
```bash
cat ~/.ssh/id_rsa.pub
# 复制输出到 https://github.com/settings/keys
```

3. 运行上传脚本，选择选项 2

### 3. 手动上传（如果脚本失败）

如果脚本有问题，你可以手动操作：

```bash
# 1. 初始化 git（如果需要）
git init

# 2. 添加所有文件
git add .

# 3. 提交
git commit -m "Initial commit: P2P file transfer application"

# 4. 添加远程仓库
git remote add origin https://github.com/你的用户名/p2p-transfer.git

# 5. 推送
git push -u origin master
```

---

## 方法二：直接在 GitHub 网页上传

1. 访问 https://github.com/new
2. 填写仓库信息：
   - Repository name: `p2p-transfer`
   - Description: `Direct peer-to-peer file transfer without server`
   - 选择 Public 或 Private
   - 不要勾选 "Initialize this repository with a README"
3. 点击 "Create repository"
4. 按照网页上的指示：
   ```bash
   cd p2p_transfer
   git init
   git add .
   git commit -m "Initial commit"
   git remote add origin https://github.com/你的用户名/p2p-transfer.git
   git branch -M main
   git push -u origin main
   ```

---

## 方法三：使用 GitHub Desktop

1. 下载 GitHub Desktop: https://desktop.github.com/
2. 登录 GitHub 账号
3. File → Add Local Repository
4. 选择 `p2p_transfer` 文件夹
5. 点击 "Publish repository"
6. 设置仓库名和可见性
7. 完成！

---

## 常见问题

### Q: 推送被拒绝？

**错误信息：** `remote: Permission to user/repo.git denied`

**解决方案：**
- 检查 Token 是否有 `repo` 权限
- 检查仓库名是否正确
- 如果用 SSH，确保 SSH Key 已添加到 GitHub

### Q: 仓库已存在？

**解决方案：**
- 换一个仓库名
- 或先删除 GitHub 上的仓库：
  ```bash
  # 使用 GitHub API 删除
  curl -X DELETE -H "Authorization: token YOUR_TOKEN" \
    https://api.github.com/repos/USERNAME/REPO
  ```

### Q: 大文件推送失败？

**解决方案：**
- 确保 `.gitignore` 包含编译文件
- 检查文件大小（GitHub 单文件限制 100MB）
- 使用 Git LFS 处理大文件：
  ```bash
  git lfs install
  git lfs track "*.zip"
  ```

---

## 上传后的下一步

1. 添加 LICENSE（推荐 MIT License）
2. 添加徽章（Build, License 等）
3. 完善 README.md
4. 创建 releases
5. 开启 Issues 和 Discussions

---

## 快速命令参考

```bash
# 查看状态
git status

# 查看远程仓库
git remote -v

# 强制推送（谨慎使用）
git push -f origin master

# 更新远程 URL
git remote set-url origin NEW_URL

# 查看提交历史
git log --oneline

# 创建标签
git tag v1.0.0
git push origin v1.0.0
```

祝你上传成功！🚀

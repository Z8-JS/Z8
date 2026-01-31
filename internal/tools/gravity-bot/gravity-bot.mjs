import git from 'isomorphic-git';
import http from 'isomorphic-git/http/node';
import fs from 'fs';
import path from 'path';
import dotenv from 'dotenv';

dotenv.config();

const GITHUB_TOKEN = process.env.GITHUB_TOKEN;
const REPO_OWNER = process.env.REPO_OWNER || 'Z8-JS';
const REPO_NAME = process.env.REPO_NAME || 'Z8';
const BOT_NAME = 'Gravity Bot';
const BOT_EMAIL = 'gravity-bot@z8.internal';

const dir = path.resolve('d:/Z8'); // Gốc workspace

async function pushChanges(branchName, commitMessage) {
    if (!GITHUB_TOKEN || GITHUB_TOKEN.length < 10) {
        throw new Error('Bạn chưa dán Token hợp lệ vào tệp .env');
    }

    console.log(`🚀 [Gravity Bot] Đang chuẩn bị đẩy Z8-app lên ${REPO_OWNER}/${REPO_NAME}...`);

    const gitdir = path.join(dir, '.git');
    
    // 1. Chỉ Stage các thay đổi trong Z8-app và các tệp cấu hình quan trọng
    const status = await git.statusMatrix({ fs, dir });
    for (const [file, head, workdir, stage] of status) {
        // Chỉ chấp nhận Z8-app và các tệp config gốc, bỏ qua toàn bộ rác build/lib
        const isAppFile = file.startsWith('Z8-app/') || file === '.gitignore' || file === 'README.md';
        const isJunk = file.includes('node_modules') || file.includes('.obj') || file.includes('.exe') || file.includes('.lib') || file.includes('include/');
        
        if (isAppFile && !isJunk) {
            if (workdir === 0 && head !== 0) {
                await git.remove({ fs, dir, filepath: file });
            } else if (workdir !== head) {
                await git.add({ fs, dir, filepath: file });
            }
        }
    }

    // 2. Commit với danh tính của Bot
    const sha = await git.commit({
        fs,
        dir,
        author: { name: BOT_NAME, email: BOT_EMAIL },
        committer: { name: BOT_NAME, email: BOT_EMAIL },
        message: commitMessage,
    });

    console.log(`✅ [Gravity Bot] Đã tạo commit: ${sha}`);

    // 3. Đẩy code bằng Token (OAuth2)
    const url = `https://github.com/${REPO_OWNER}/${REPO_NAME}.git`;
    
    await git.push({
        fs,
        http,
        dir,
        url,
        ref: branchName,
        force: true,
        onAuth: () => ({ 
            username: GITHUB_TOKEN, // Một số Git server dùng Token ở đây
            password: '' 
        }),
    }).then(() => {
        console.log(`🌍 [Gravity Bot] THÀNH CÔNG! Code đã được đẩy lên GitHub bằng tài khoản Bot.`);
    }).catch(err => {
        // Không in full lỗi để tránh lộ Token nếu có
        console.error(`❌ [Gravity Bot] Thất bại: ${err.message}`);
        console.log(`💡 Gợi ý: Hãy đảm bảo Token trong .env có quyền 'repo' và Bot đã được mời vào organization Z8-JS.`);
    });
}

const branch = process.argv[2] || 'feat/fs-implementation';
const msg = process.argv[3] || 'Update from Gravity Bot';
pushChanges(branch, msg).catch(e => console.error(e.message));

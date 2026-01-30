const user = {
    id: 1,
    profile: {
        name: "Dev2026",
        settings: {
            theme: "dark",
            notifications: {
                email: true,
                sms: false // Tầng này thường bị console.log ẩn đi thành [Object]
            }
        }
    }
};

console.log("bình thường:");
console.log(user);
console.log("dir:");
// Sử dụng console.dir để xem toàn bộ (depth: null nghĩa là không giới hạn độ sâu)
console.dir(user, { depth: null, colors: true });
console.log("dirxml:");
console.dirxml(user);
console.info("test")
console.dirxml("test")
console.log("Get ready...");
console.time("Heavy task");
// Chạy một vòng lặp lớn
for(let i=0; i<1_000_000_000; i++) {}
console.timeLog("Heavy task"); // Xem tiến độ
console.timeEnd("Heavy task"); // Kết thúc và xem tổng thời gian
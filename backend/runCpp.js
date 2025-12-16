import { spawn } from "child_process";
import path from "path";

export function runCpp(args = []) {
  // Absolute path to your main.exe
  const exePath = path.resolve(
    "D:/3RD SEMESTER/Data Structures and Algorithms/dsa search engine/DSA-Searh-Engine/build/main.exe"
  );

  return new Promise((resolve, reject) => {
    const cppProcess = spawn(exePath, args);

    let output = "";
    let error = "";

    cppProcess.stdout.on("data", (data) => {
      output += data.toString();
    });

    cppProcess.stderr.on("data", (data) => {
      error += data.toString();
    });

    cppProcess.on("close", (code) => {
      if (code === 0) {
        try {
          resolve(JSON.parse(output)); // assuming your exe outputs valid JSON
        } catch (err) {
          reject(`Failed to parse JSON: ${err}\nOutput: ${output}`);
        }
      } else {
        reject(`C++ process exited with code ${code}\nError: ${error}`);
      }
    });
  });
}

// Finds .cpp solvers recursively inside category folders under Solver/.
// The category is the full relative folder path, for example Submissions/Normal.
// Solver/dist is build output and is excluded from discovery.
import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import path from "node:path";

/** Folders of Solver/ that are not categories. */
const NOT_CATEGORIES = new Set(["dist"]);

const COMMENT_OR_STRING = /\/\*[\s\S]*?\*\/|\/\/[^\n]*|"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*'/g;

/** The source with comments blanked and the text of strings and characters emptied, so that only code is left to search. */
export function codeOf(source) {
  return source.replace(COMMENT_OR_STRING, (m) => (m.startsWith("/") ? " " : m[0] === '"' ? '""' : "''"));
}

/** Whether the solver takes snapshots: it uses NQ_SNAPSHOT or calls snapshot(). */
export function usesSnapshots(source) {
  return /\bNQ_SNAPSHOT\b|\bsnapshot\s*\(/.test(codeOf(source));
}

const byCode = (a, b) => (a < b ? -1 : a > b ? 1 : 0);

/**
 * The solvers under `<root>/Solver`, sorted by category and then name. `file` is the path of the source from `root`, with
 * forward slashes, and `source` is its full path.
 */
export function discoverSolvers(root) {
  const base = path.join(root, "Solver");
  if (!existsSync(base)) return [];
  const solvers = [];
  function visit(category) {
    const dir = path.join(base, category);
    for (const entry of readdirSync(dir, { withFileTypes: true })) {
      if (entry.name.startsWith(".")) continue;
      if (entry.isDirectory()) {
        visit(`${category}/${entry.name}`);
        continue;
      }
      if (!entry.isFile() || !entry.name.endsWith(".cpp")) continue;
      const source = path.join(dir, entry.name);
      const name = entry.name.slice(0, -".cpp".length);
      solvers.push({
        id: `${category}/${name}`,
        category,
        name,
        file: `Solver/${category}/${entry.name}`,
        source,
        snapshots: usesSnapshots(readFileSync(source, "utf8")),
      });
    }
  }
  for (const category of readdirSync(base)) {
    if (NOT_CATEGORIES.has(category) || category.startsWith(".")) continue;
    if (statSync(path.join(base, category)).isDirectory()) visit(category);
  }
  return solvers.sort((a, b) => byCode(a.category, b.category) || byCode(a.name, b.name));
}

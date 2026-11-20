// ---------- helpers ----------
const DAYS = ["Mon", "Tue", "Wed", "Thu", "Fri"];
const $ = s => document.querySelector(s);
const esc = s => String(s).replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const fmt = m => String(Math.floor(m / 60)).padStart(2, "0") + ":" + String(m % 60).padStart(2, "0");
const toMin = t => { const [h, m] = t.split(":"); return h * 60 + +m; };
const timeOf = c => fmt(c.start) + "–" + fmt(c.end);

const classes = DATA.map(([course, teacher, section, room, day, start, end], id) =>
  ({ id, course, teacher, section, room, day: DAYS[day], start, end }));

const overlaps = (a, b) => a.day === b.day && a.start < b.end && b.start < a.end;
const isShared = (a, b) => a.course === b.course && a.teacher === b.teacher && a.room === b.room && a.start === b.start && a.end === b.end;

let toastTimer;
function toast(msg) {
  const t = $("#toast"); t.textContent = msg; t.classList.add("show");
  clearTimeout(toastTimer); toastTimer = setTimeout(() => t.classList.remove("show"), 2600);
}

// ---------- singly linked list: My Timetable ----------
class Node1 { constructor(data) { this.data = data; this.next = null; } }
class LinkedList {
  constructor() { this.head = null; this.size = 0; }
  addEnd(data) {
    const node = new Node1(data);
    if (!this.head) this.head = node;
    else { let cur = this.head; while (cur.next) cur = cur.next; cur.next = node; }
    this.size++;
  }
  removeAt(pos) {                       // pos starts at 0
    if (pos < 0 || pos >= this.size) return false;
    if (pos === 0) this.head = this.head.next;
    else { let prev = this.head; for (let i = 1; i < pos; i++) prev = prev.next; prev.next = prev.next.next; }
    this.size--; return true;
  }
  toArray() { const out = []; for (let cur = this.head; cur; cur = cur.next) out.push(cur.data); return out; }
}
const mine = new LinkedList();
try { JSON.parse(localStorage.getItem("cf-mine") || "[]").forEach(id => classes[id] && mine.addEnd(classes[id])); } catch (e) {}
const saveMine = () => { try { localStorage.setItem("cf-mine", JSON.stringify(mine.toArray().map(c => c.id))); } catch (e) {} };

function addToMine(c) {
  const list = mine.toArray();
  if (list.some(x => x.id === c.id)) return toast("Already in your timetable");
  const bad = list.find(x => overlaps(x, c));
  if (bad) return toast("Clash with " + bad.course + " (" + bad.day + " " + timeOf(bad) + ")");
  mine.addEnd(c); saveMine(); renderMine(); toast("Added " + c.course);
}

function renderMine() {
  const list = mine.toArray().map((c, pos) => ({ c, pos }));
  if (!list.length) { $("#mine").innerHTML = '<p class="hint">Nothing here yet.</p>'; return; }
  list.sort((a, b) => DAYS.indexOf(a.c.day) - DAYS.indexOf(b.c.day) || a.c.start - b.c.start);
  let html = "", lastDay = "";
  for (const { c, pos } of list) {
    if (c.day !== lastDay) { html += '<div class="day">' + c.day + "</div>"; lastDay = c.day; }
    html += '<div class="row"><span><b>' + timeOf(c) + "</b> " + esc(c.course) + " · " + esc(c.section) + " · " + esc(c.room) +
      '</span><button class="btn ghost small" data-rm="' + pos + '">Remove</button></div>';
  }
  $("#mine").innerHTML = html;
}
$("#mine").addEventListener("click", e => {
  const pos = e.target.dataset.rm;
  if (pos !== undefined) { mine.removeAt(+pos); saveMine(); renderMine(); }
});

// ---------- sorting (comparison counter included) ----------
let compares = 0;
const keyCompare = {
  time: (a, b) => DAYS.indexOf(a.day) - DAYS.indexOf(b.day) || a.start - b.start,
  course: (a, b) => a.course.localeCompare(b.course) || keyCompare.time(a, b),
  teacher: (a, b) => a.teacher.localeCompare(b.teacher) || keyCompare.time(a, b)
};
const makeCmp = key => (a, b) => { compares++; return keyCompare[key](a, b); };

const SORTS = {
  bubble(list, cmp) {
    const a = [...list];
    for (let i = 0; i < a.length - 1; i++) {
      let swapped = false;
      for (let j = 0; j < a.length - 1 - i; j++) {
        if (cmp(a[j], a[j + 1]) > 0) { [a[j], a[j + 1]] = [a[j + 1], a[j]]; swapped = true; }
      }
      if (!swapped) break;
    }
    return a;
  },
  insertion(list, cmp) {
    const a = [...list];
    for (let i = 1; i < a.length; i++) {
      const current = a[i]; let j = i - 1;
      while (j >= 0 && cmp(current, a[j]) < 0) { a[j + 1] = a[j]; j--; }
      a[j + 1] = current;
    }
    return a;
  },
  selection(list, cmp) {
    const a = [...list];
    for (let i = 0; i < a.length - 1; i++) {
      let smallest = i;
      for (let j = i + 1; j < a.length; j++) if (cmp(a[j], a[smallest]) < 0) smallest = j;
      if (smallest !== i) [a[i], a[smallest]] = [a[smallest], a[i]];
    }
    return a;
  },
  merge(list, cmp) {                    // recursion: split, sort halves, merge
    if (list.length < 2) return list;
    const mid = list.length >> 1;
    const left = SORTS.merge(list.slice(0, mid), cmp), right = SORTS.merge(list.slice(mid), cmp);
    const out = []; let i = 0, j = 0;
    while (i < left.length && j < right.length) out.push(cmp(right[j], left[i]) < 0 ? right[j++] : left[i++]);
    return out.concat(left.slice(i), right.slice(j));
  },
  quick(list, cmp) {                    // recursion: partition around a pivot
    const a = [...list];
    const sortPart = (low, high) => {
      if (low >= high) return;
      const pivot = a[high]; let i = low - 1;
      for (let j = low; j < high; j++) if (cmp(a[j], pivot) < 0) { i++; [a[i], a[j]] = [a[j], a[i]]; }
      [a[i + 1], a[high]] = [a[high], a[i + 1]];
      sortPart(low, i); sortPart(i + 2, high);
    };
    sortPart(0, a.length - 1);
    return a;
  }
};

// ---------- browse tab ----------
let shown = [], limit = 100;
function runBrowse(resetLimit) {
  if (resetLimit) limit = 100;
  const q = $("#q").value.trim().toLowerCase(), field = $("#field").value;
  const hits = [];
  for (let i = 0; i < classes.length; i++) {            // linear search
    if (!q || classes[i][field].toLowerCase().includes(q)) hits.push(classes[i]);
  }
  compares = 0;
  const t0 = performance.now();
  shown = SORTS[$("#algo").value](hits, makeCmp($("#sortKey").value));
  const ms = (performance.now() - t0).toFixed(1);
  $("#info").textContent = hits.length + " of " + classes.length + " classes found · " + $("#algo").selectedOptions[0].text +
    ": " + compares.toLocaleString() + " comparisons, " + ms + " ms";
  $("#results").innerHTML = shown.slice(0, limit).map(c =>
    "<tr><td>" + esc(c.course) + "</td><td>" + esc(c.teacher) + "</td><td>" + esc(c.section) + "</td><td>" + esc(c.room) +
    "</td><td>" + c.day + "</td><td>" + timeOf(c) + '</td><td><button class="btn ghost small" data-add="' + c.id + '">+ Mine</button></td></tr>').join("");
  $("#more").hidden = shown.length <= limit;
}
["input", "change"].forEach(ev => ["#q", "#field", "#sortKey", "#algo"].forEach(s => $(s).addEventListener(ev, () => runBrowse(true))));
$("#more").onclick = () => { limit += 200; runBrowse(false); };
$("#results").addEventListener("click", e => { const id = e.target.dataset.add; if (id !== undefined) addToMine(classes[id]); });

// ---------- binary search tree: course index ----------
class TreeNode { constructor(name) { this.name = name; this.count = 1; this.left = null; this.right = null; } }
class CourseTree {
  constructor() { this.root = null; }
  insert(name) { this.root = this._insert(this.root, name); }
  _insert(node, name) {                 // recursion
    if (!node) return new TreeNode(name);
    const a = name.toLowerCase(), b = node.name.toLowerCase();
    if (a === b) node.count++;
    else if (a < b) node.left = this._insert(node.left, name);
    else node.right = this._insert(node.right, name);
    return node;
  }
  search(name) {
    const key = name.trim().toLowerCase(); let cur = this.root, steps = 0;
    while (cur) {
      steps++;
      const b = cur.name.toLowerCase();
      if (key === b) return { node: cur, steps };
      cur = key < b ? cur.left : cur.right;
    }
    return { node: null, steps };
  }
  inOrder(node = this.root, out = []) { // left, node, right = A to Z
    if (node) { this.inOrder(node.left, out); out.push(node); this.inOrder(node.right, out); }
    return out;
  }
}
const tree = new CourseTree();
classes.forEach(c => tree.insert(c.course));
$("#courseList").innerHTML = tree.inOrder().map(n => '<option value="' + esc(n.name) + '">').join("");

// ---------- planner: backtracking ----------
const MAX_PLANS = 5000;
let chosen = [], lastPlans = [];
function drawChosen() {
  $("#chosen").innerHTML = chosen.map((name, i) => '<span class="chip">' + esc(name) + ' <button data-drop="' + i + '" aria-label="Remove">×</button></span>').join("");
}
$("#addCourse").onclick = () => {
  const { node, steps } = tree.search($("#courseIn").value);
  if (!node) return toast("Course not found");
  if (chosen.includes(node.name)) return toast("Already added");
  if (chosen.length >= 8) return toast("Maximum 8 courses");
  chosen.push(node.name); $("#courseIn").value = ""; drawChosen();
  toast("Found in " + steps + " tree step" + (steps > 1 ? "s" : ""));
};
$("#chosen").addEventListener("click", e => { const i = e.target.dataset.drop; if (i !== undefined) { chosen.splice(i, 1); drawChosen(); } });

const groupClash = (a, b) => a.some(x => b.some(y => overlaps(x, y)));

function search(groups, level, picked, out) {
  if (out.count >= MAX_PLANS) return;
  if (level === groups.length) {                       // base case: every course has a section
    out.count++;
    if (out.plans.length < 3) out.plans.push([...picked]);
    return;
  }
  for (const option of groups[level]) {
    if (picked.some(p => groupClash(p.rows, option.rows))) { out.backtracks++; continue; }   // clash: skip
    picked.push(option);                                // choose
    search(groups, level + 1, picked, out);             // go deeper
    picked.pop();                                       // undo (backtrack)
  }
}

$("#findPlans").onclick = () => {
  if (!chosen.length) return toast("Add at least one course first");
  const groups = chosen.map(name => {
    const bySection = {};
    classes.filter(c => c.course === name).forEach(c => (bySection[c.section] = bySection[c.section] || []).push(c));
    return Object.entries(bySection).map(([section, rows]) => ({ course: name, section, rows }));
  });
  const out = { count: 0, backtracks: 0, plans: [] };
  search(groups, 0, [], out);
  lastPlans = out.plans;
  $("#planInfo").textContent = out.count
    ? (out.count >= MAX_PLANS ? MAX_PLANS + "+" : out.count) + " clash-free plan(s) · " + out.backtracks.toLocaleString() + " clashes avoided by backtracking · showing " + out.plans.length
    : "No clash-free plan exists for these courses (" + out.backtracks + " backtracks).";
  $("#plans").innerHTML = out.plans.map((plan, i) =>
    '<div class="plan"><b>Plan ' + (i + 1) + '</b><ul>' + plan.map(p => "<li>" + esc(p.course) + " – <b>" + esc(p.section) + "</b> (" +
      p.rows.map(r => r.day + " " + timeOf(r)).join(", ") + ')</li>').join("") +
    '</ul><button class="btn small" data-plan="' + i + '">Add to my timetable</button></div>').join("");
};
$("#plans").addEventListener("click", e => {
  const i = e.target.dataset.plan;
  if (i === undefined) return;
  let added = 0;
  lastPlans[i].forEach(p => p.rows.forEach(r => {
    if (!mine.toArray().some(x => x.id === r.id || overlaps(x, r))) { mine.addEnd(r); added++; }
  }));
  saveMine(); renderMine(); toast(added + " class(es) added");
});

// ---------- free rooms and clash report ----------
$("#fDay").innerHTML = DAYS.map(d => "<option>" + d + "</option>").join("");
$("#fGo").onclick = () => {
  const slot = { day: $("#fDay").value, start: toMin($("#fStart").value || "0:0"), end: toMin($("#fEnd").value || "0:0") };
  if (slot.start >= slot.end) return toast("End time must be after start time");
  const rooms = [...new Set(classes.map(c => c.room))].sort();
  const busy = new Set(classes.filter(c => overlaps(slot, c)).map(c => c.room));
  const free = rooms.filter(r => !busy.has(r));
  $("#fInfo").textContent = free.length + " of " + rooms.length + " rooms are free on " + slot.day + " " + fmt(slot.start) + "–" + fmt(slot.end);
  $("#fList").innerHTML = free.map(r => '<span class="chip free">' + esc(r) + "</span>").join("");
};

$("#cGo").onclick = () => {
  const type = $("#cType").value, pairs = [];
  for (const day of DAYS) {                               // compare only classes on the same day
    const list = classes.filter(c => c.day === day);
    for (let i = 0; i < list.length; i++) for (let j = i + 1; j < list.length; j++) {
      if (list[i][type] === list[j][type] && overlaps(list[i], list[j]) && !isShared(list[i], list[j])) pairs.push([list[i], list[j]]);
    }
  }
  $("#cInfo").textContent = pairs.length + " " + type + " clash(es) found" + (pairs.length > 40 ? " · showing the first 40" : "");
  $("#cList").innerHTML = pairs.slice(0, 40).map(([a, b]) => '<div class="clash"><b>' + esc(a[type]) + "</b> on " + a.day + "<br>" +
    esc(a.course) + " (" + esc(a.section) + ", " + timeOf(a) + ")<br>" + esc(b.course) + " (" + esc(b.section) + ", " + timeOf(b) + ")</div>").join("");
};

// ---------- DSA map ----------
const DSA = [
  ["Singly linked list", "My timetable: add at end, remove by position, traverse", "✓"],
  ["Doubly linked list", "Class browser (next / previous) in main.cpp", "main.cpp"],
  ["Circular linked list", "Week-day cycle, Sat wraps to Mon, in main.cpp", "main.cpp"],
  ["Stack (LIFO)", "Undo last change in main.cpp", "main.cpp"],
  ["Queue (FIFO)", "Room-change requests in main.cpp", "main.cpp"],
  ["Linear search", "Browse tab search over every class", "✓"],
  ["Binary search", "Course lookup by name in main.cpp; the website uses the tree below", "main.cpp"],
  ["Binary search tree", "Course index: insert, search with step count, in-order A–Z", "✓"],
  ["Recursion", "Merge sort, quick sort, tree insert and in-order, backtracking", "✓"],
  ["Backtracking", "Planner: choose a section, go deeper, undo on clash", "✓"],
  ["Sorting (5 algorithms)", "Bubble, insertion, selection, merge, quick with comparison counts", "✓"],
  ["Graph + BFS", "Clash graph groups in main.cpp", "main.cpp"]
];
$("#dsaRows").innerHTML = DSA.map(r => "<tr><td>" + r[0] + "</td><td style='white-space:normal'>" + r[1] + "</td><td>" + r[2] + "</td></tr>").join("");

// ---------- tabs ----------
$("#tabs").addEventListener("click", e => {
  const tab = e.target.dataset.tab;
  if (!tab) return;
  document.querySelectorAll("nav button").forEach(b => b.classList.toggle("on", b === e.target));
  ["browse", "plan", "rooms", "dsa"].forEach(id => $("#" + id).hidden = id !== tab);
});

runBrowse(true);
renderMine();

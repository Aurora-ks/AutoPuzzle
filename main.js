document.addEventListener('DOMContentLoaded', () => {
    // --- 全局常量和变量 ---
    let GRID_ROWS = 5;
    let GRID_COLS = 6;
    const gridContainer = document.getElementById('grid-container');
    const pieceControlsContainer = document.getElementById('piece-controls');
    const solveButton = document.getElementById('solve-button');
    const resetButton = document.getElementById('reset-button');
    const messageArea = document.getElementById('message-area');
    const gridRowsInput = document.getElementById('grid-rows');
    const gridColsInput = document.getElementById('grid-cols');

    let gridState = Array(GRID_ROWS).fill(0).map(() => Array(GRID_COLS).fill(0)); // 0: 可填充, -1: 障碍

    // 11种拼图块的基础形状定义
    const PIECE_DEFINITIONS = [
        {id: 1, name: '方形', coords: [[0, 0], [0, 1], [1, 0], [1, 1]]},
        {id: 2, name: '长条', coords: [[0, 0], [0, 1], [0, 2], [0, 3]]},
        {id: 3, name: 'Z形1', coords: [[0, 1], [1, 1], [1, 0], [2, 0]]},
        {id: 4, name: 'Z形2', coords: [[0, 0], [1, 0], [1, 1], [2, 1]]},
        {id: 5, name: 'L形1', coords: [[0, 0], [1, 0], [1, 1], [1, 2]]},
        {id: 6, name: 'L形2', coords: [[0, 0], [0, 1], [0, 2], [1, 0]]},
        {id: 7, name: 'T形', coords: [[0, 0], [0, 1], [0, 2], [1, 1]]},
        {id: 8, name: '十字', coords: [[0, 1], [1, 1], [1, 2], [1, 0], [2, 1]]},
        {id: 9, name: '单点', coords: [[0, 0]]},
        {id: 10, name: '双点', coords: [[0, 0], [0, 1]]},
        {id: 11, name: 'L形3', coords: [[0, 0], [0, 1], [1, 0]]},
    ];

    // 为每个拼图块定义一个颜色
    const PIECE_COLORS = [
        '#FFFFFF', // 0 - 空白
        '#f87171', // 1
        '#fb923c', // 2
        '#facc15', // 3
        '#a3e635', // 4
        '#4ade80', // 5
        '#34d399', // 6
        '#2dd4bf', // 7
        '#60a5fa', // 8
        '#818cf8', // 9
        '#c084fc', // 10
        '#f472b6', // 11
    ];

    // 预先计算好的所有拼图块及其唯一旋转形态
    const allPieceShapes = [
        {
            "id": 1,
            "shapes": [
                [
                    [0, 0],
                    [0, 1],
                    [1, 0],
                    [1, 1]
                ]
            ]
        },
        {
            "id": 2,
            "shapes": [
                [
                    [0, 0],
                    [0, 1],
                    [0, 2],
                    [0, 3]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [2, 0],
                    [3, 0]
                ]
            ]
        },
        {
            "id": 3,
            "shapes": [
                [
                    [0, 0],
                    [1, 0],
                    [1, 1],
                    [2, 1]
                ],
                [
                    [0, 1],
                    [0, 2],
                    [1, 0],
                    [1, 1]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [1, -1],
                    [2, -1]
                ],
                [
                    [0, 0],
                    [0, 1],
                    [-1, 1],
                    [-1, 2]
                ]
            ]
        },
        {
            "id": 4,
            "shapes": [
                [
                    [0, 0],
                    [1, 0],
                    [1, -1],
                    [2, -1]
                ],
                [
                    [0, 1],
                    [0, 0],
                    [1, 0],
                    [1, -1]
                ],
                [
                    [0, 1],
                    [1, 1],
                    [1, 0],
                    [2, 0]
                ],
                [
                    [0, 0],
                    [0, 1],
                    [-1, 1],
                    [-1, 2]
                ]
            ]
        },
        {
            "id": 5,
            "shapes": [
                [
                    [0, 0],
                    [1, 0],
                    [1, 1],
                    [1, 2]
                ],
                [
                    [0, 2],
                    [1, 0],
                    [1, 1],
                    [1, 2]
                ],
                [
                    [0, 0],
                    [0, 1],
                    [0, 2],
                    [1, 2]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [2, 0],
                    [2, 1]
                ]
            ]
        },
        {
            "id": 6,
            "shapes": [
                [
                    [0, 0],
                    [0, 1],
                    [0, 2],
                    [1, 0]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [2, 0],
                    [2, -1]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [1, 1],
                    [1, 2]
                ],
                [
                    [0, 2],
                    [1, 0],
                    [1, 1],
                    [1, 2]
                ]
            ]
        },
        {
            "id": 7,
            "shapes": [
                [
                    [0, 0],
                    [0, 1],
                    [0, 2],
                    [1, 1]
                ],
                [
                    [0, 1],
                    [1, 0],
                    [1, 1],
                    [2, 1]
                ],
                [
                    [0, 1],
                    [1, 0],
                    [1, 1],
                    [1, 2]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [1, 1],
                    [2, 0]
                ]
            ]
        },
        {
            "id": 8,
            "shapes": [
                [
                    [0, 1],
                    [1, 0],
                    [1, 1],
                    [1, 2],
                    [2, 1]
                ]
            ]
        },
        {
            "id": 9,
            "shapes": [
                [
                    [0, 0]
                ]
            ]
        },
        {
            "id": 10,
            "shapes": [
                [
                    [0, 0],
                    [0, 1]
                ],
                [
                    [0, 0],
                    [1, 0]
                ]
            ]
        },
        {
            "id": 11,
            "shapes": [
                [
                    [0, 0],
                    [0, 1],
                    [1, 0]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [1, -1]
                ],
                [
                    [0, 0],
                    [1, 0],
                    [0, -1]
                ],
                [
                    [0, 0],
                    [0, 1],
                    [-1, 1]
                ]
            ]
        }
    ];


    // --- 初始化函数 ---
    function initialize() {
        createGrid();
        createPieceControls();
        solveButton.addEventListener('click', runSolver);
        resetButton.addEventListener('click', resetAll);
        gridRowsInput.addEventListener('change', updateGridSize);
        gridColsInput.addEventListener('change', updateGridSize);
    }

    function updateGridSize() {
        GRID_ROWS = parseInt(gridRowsInput.value) || 5;
        GRID_COLS = parseInt(gridColsInput.value) || 6;
        gridState = Array(GRID_ROWS).fill(0).map(() => Array(GRID_COLS).fill(0));
        createGrid();
        messageArea.innerHTML = '';
    }

    // --- UI 创建函数 ---
    function createGrid() {
        gridContainer.innerHTML = '';
        gridContainer.style.gridTemplateColumns = `repeat(${GRID_COLS}, 1fr)`;
        gridContainer.style.aspectRatio = `${GRID_COLS}/${GRID_ROWS}`;

        for (let r = 0; r < GRID_ROWS; r++) {
            for (let c = 0; c < GRID_COLS; c++) {
                const cell = document.createElement('div');
                cell.dataset.row = r;
                cell.dataset.col = c;
                cell.className = 'w-full h-full bg-white rounded-md cursor-pointer transition-colors duration-200 grid-cell';
                cell.addEventListener('click', toggleCellState);
                gridContainer.appendChild(cell);
            }
        }
    }

    function createPieceControls() {
        pieceControlsContainer.innerHTML = '';
        PIECE_DEFINITIONS.forEach(p => {
            const control = document.createElement('div');
            control.className = 'flex flex-col items-center p-2 border rounded-lg';

            const preview = createPiecePreview(p.coords);

            const label = document.createElement('label');
            label.className = 'text-sm font-medium mt-2';
            label.textContent = `块 ${p.id}`;

            const input = document.createElement('input');
            input.type = 'number';
            input.id = `piece-count-${p.id}`;
            input.min = 0;
            input.value = 5; // 默认值
            input.className = 'w-16 text-center mt-1 p-1 border rounded-md';

            control.appendChild(preview);
            control.appendChild(label);
            control.appendChild(input);
            pieceControlsContainer.appendChild(control);
        });
    }

    function createPiecePreview(coords) {
        const previewContainer = document.createElement('div');
        previewContainer.className = 'piece-preview';
        const cells = Array(16).fill(0).map(() => {
            const cell = document.createElement('div');
            cell.className = 'piece-preview-cell';
            return cell;
        });

        coords.forEach(([r, c]) => {
            // 调整预览网格的坐标，使其从 (0,0) 开始显示
            const minR = Math.min(...coords.map(coord => coord[0]));
            const minC = Math.min(...coords.map(coord => coord[1]));

            const displayR = r - minR;
            const displayC = c - minC;

            if (displayR >= 0 && displayR < 4 && displayC >= 0 && displayC < 4) {
                cells[displayR * 4 + displayC].style.backgroundColor = PIECE_COLORS[1]; // 预览使用通用颜色
            }
        });

        cells.forEach(c => previewContainer.appendChild(c));
        return previewContainer;
    }

    // --- 事件处理函数 ---
    function toggleCellState(event) {
        const cell = event.target;
        const r = parseInt(cell.dataset.row);
        const c = parseInt(cell.dataset.col);

        if (gridState[r][c] === 0) {
            gridState[r][c] = -1; // 设置为障碍
            cell.style.backgroundColor = '#9ca3af'; // 灰色
            cell.classList.remove('bg-white');
            cell.textContent = ''; // 清除文本
        } else if (gridState[r][c] === -1) {
            gridState[r][c] = 0; // 恢复为可填充
            cell.style.backgroundColor = '#FFFFFF';
            cell.classList.add('bg-white');
            cell.textContent = ''; // 清除文本
        }
    }

    function resetAll() {
        gridState = Array(GRID_ROWS).fill(0).map(() => Array(GRID_COLS).fill(0));
        createGrid();
        createPieceControls();
        messageArea.innerHTML = '';
        solveButton.disabled = false;
        solveButton.innerHTML = '开始搜索';
    }

    // --- 核心算法逻辑 ---

    // 2. 求解器入口
    async function runSolver() {
        // 禁用按钮并显示加载状态
        solveButton.disabled = true;
        messageArea.innerHTML = '<div class="flex justify-center items-center"><div class="loader"></div><span class="ml-4">正在全力搜索...</span></div>';

        // 确保UI有时间更新
        await new Promise(resolve => setTimeout(resolve, 50));

        const pieceCounts = PIECE_DEFINITIONS.map(p => {
            return parseInt(document.getElementById(`piece-count-${p.id}`).value) || 0;
        });

        const solution = solve(gridState, pieceCounts);

        if (solution) {
            displaySolution(solution);
            messageArea.innerHTML = '<p class="text-lg font-bold text-green-600">成功找到解决方案！</p>';
        } else {
            messageArea.innerHTML = '<p class="text-lg font-bold text-red-600">无法实现！没有找到可行的填充方案。</p>';
        }
        solveButton.disabled = false;
        solveButton.innerHTML = '开始搜索';
    }

    // 3. 递归回溯函数
    function solve(board, counts) {
        const findEmpty = () => {
            for (let r = 0; r < GRID_ROWS; r++) {
                for (let c = 0; c < GRID_COLS; c++) {
                    if (board[r][c] === 0) return [r, c];
                }
            }
            return null; // 没有空格子了，成功
        };

        const emptyCell = findEmpty();
        if (!emptyCell) {
            return board; // 找到解
        }

        const [r, c] = emptyCell;

        // 遍历所有类型的拼图块
        for (const piece of allPieceShapes) {
            if (counts[piece.id - 1] > 0) {
                // 遍历该拼图块的所有旋转形态
                for (const shape of piece.shapes) {
                    // 在尝试放置之前，检查形状的坐标（旋转后可能为负）是否可以相对 (r, c) 偏移以适应网格。
                    // 这涉及到找到使所有形状坐标相对于 (r,c) 为非负所需的最小行/列偏移量。
                    const minDr = Math.min(...shape.map(coord => coord[0]));
                    const minDc = Math.min(...shape.map(coord => coord[1]));

                    // 调整后的放置原点
                    const adjustedR = r - minDr;
                    const adjustedC = c - minDc;


                    if (canPlace(board, shape, adjustedR, adjustedC)) {
                        // 放置
                        const newBoard = place(board, shape, adjustedR, adjustedC, piece.id);
                        const newCounts = [...counts];
                        newCounts[piece.id - 1]--;

                        // 递归
                        const result = solve(newBoard, newCounts);
                        if (result) {
                            return result; // 成功，返回结果
                        }
                    }
                }
            }
        }

        return null; // 所有尝试都失败了
    }

    // 4. 辅助函数
    function canPlace(board, shape, r, c) {
        for (const [dr, dc] of shape) {
            const nr = r + dr;
            const nc = c + dc;
            // 检查边界以及单元格是否已被占用或为障碍物
            if (nr < 0 || nr >= GRID_ROWS || nc < 0 || nc >= GRID_COLS || board[nr][nc] !== 0) {
                return false;
            }
        }
        return true;
    }

    function place(board, shape, r, c, pieceId) {
        // 创建一个新板子，避免修改原始板子
        const newBoard = board.map(row => [...row]);
        for (const [dr, dc] of shape) {
            newBoard[r + dr][c + dc] = pieceId;
        }
        return newBoard;
    }

    // --- 结果展示 ---
    function displaySolution(solution) {
        const cells = gridContainer.children;
        for (let r = 0; r < GRID_ROWS; r++) {
            for (let c = 0; c < GRID_COLS; c++) {
                const cellIndex = r * GRID_COLS + c;
                const cell = cells[cellIndex];
                const pieceId = solution[r][c];

                cell.classList.remove('cursor-pointer');
                cell.removeEventListener('click', toggleCellState);

                if (pieceId > 0) {
                    cell.style.backgroundColor = PIECE_COLORS[pieceId];
                    cell.style.border = '1px solid rgba(0,0,0,0.1)';
                    cell.textContent = pieceId; // 显示方块编号
                } else if (pieceId === -1) {
                    cell.style.backgroundColor = '#9ca3af';
                    cell.textContent = ''; // 障碍物不显示编号
                } else {
                    cell.style.backgroundColor = '#FFFFFF';
                    cell.textContent = ''; // 空白格子不显示编号
                }
            }
        }
    }

    // --- 启动应用 ---
    initialize();
});
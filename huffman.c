#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct Node 
{ 
	unsigned char	data; 
	int		freq; 
	struct	Node *left;
	struct	Node *right; 
} tNode;

typedef struct
{
	int	last;
	int	capacity;
	tNode **heapArr;
} HEAP;

////////////////////////////////////////////////////////////////////////////////
// 허프만 코드 메모리 해제
void free_huffman_code( char *codes[]) {
	int i;
	for (i = 0; i < 256; i++) free(codes[i]);
}

// 텍스트 파일을 허프만 코드를 이용하여 바이터리 파일로 인코딩
// return value : 인코딩된 파일의 바이트 수
int encoding( char *codes[], FILE *infp, FILE *outfp) {
	int c, size = 0;
	while ((c = fgetc(infp)) != EOF) {
		fputs(codes[c], outfp);
		size += strlen(codes[c]);
	}
	fflush(outfp);
	return size / 8;
}

// 바이너리 파일을 허프만 트리를 이용하여 텍스트 파일로 디코딩
void decoding( tNode *root, FILE *infp, FILE *outfp) {
	int c = 0;
	tNode *cursor = root;
	while ((c = fgetc(infp)) != EOF) {
		cursor = (c == '0') ? cursor->left : cursor->right;
		if (cursor->left || cursor->right) continue;

		fputc(cursor->data, outfp);
		cursor = root;
	}
	fflush(outfp);
}

// 파일에 속한 각 문자(바이트)의 빈도 저장
// return value : 파일에서 읽은 바이트 수
int read_chars( FILE *fp, int *ch_freq) {
    int c;
    size_t size = 0;
    while ((c = fgetc(fp)) != EOF) {
        ch_freq[c]++;
        size++;
    }
    return size;
}

// 새로운 노드를 생성
// 좌/우 subtree가 NULL이고 빈도값(freq)이 저장됨
// return value : 노드의 포인터
tNode *newNode(char data, int freq) {
    tNode *node = (tNode *)malloc(sizeof(tNode));
    node->left = NULL;
    node->right = NULL;
    node->data = data;
    node->freq = freq;
    return node;
}

// 힙 생성
// 배열을 위한 메모리 할당 (capacity)
// last = -1
HEAP *heapCreate( int capacity) {
    HEAP *heap;
	
	heap = (HEAP *)malloc(sizeof(HEAP));
	if (!heap) return NULL;

	int i;
	heap->last = -1;
	heap->capacity = capacity;
	heap->heapArr = (tNode **)malloc(sizeof(tNode *) * capacity);
	for (i = 0; i < capacity; i++) heap->heapArr[i] = (tNode *)malloc(sizeof(tNode));

	if (heap->heapArr == NULL)
	{
		fprintf( stderr, "Error : not enough memory!\n");
		free( heap);
		return NULL;
	}
	return heap;
}

// 최소힙 유지
void _reheapUp( HEAP *heap, int index) {
    tNode **arr = heap->heapArr;
	int parent;
	
	while(1)
	{
		if (index == 0) return;
		
		parent = (index - 1) / 2;
		
		if (arr[index]->freq < arr[parent]->freq) {
			tNode *temp = arr[index];
			arr[index] = arr[parent];
			arr[parent] = temp;
			
			index = parent;
		}
		else return;
	}
}

// 힙에 원소 삽입
// _reheapUp 함수 호출
int heapInsert( HEAP *heap, tNode *data) {
    if (heap->last == heap->capacity - 1)
		return 0;
	
	heap->heapArr[++heap->last] = data;

	_reheapUp(heap, heap->last);
	
	return 1;
}

// 최소힙 유지
void _reheapDown( HEAP *heap, int index) {
    tNode **arr = heap->heapArr;
	tNode *leftData;
	tNode *rightData;
	int noright = 0;
	int smallestIndex; 
	
	while(1)
	{
		if ((index * 2 + 1) > heap->last) return; // leaf node (there is no left subtree)
		leftData = arr[index * 2 + 1];

		if (index * 2 + 2 <= heap->last) rightData = arr[index * 2 + 2];
		else noright = 1;
		
		if (noright || leftData->freq < rightData->freq) smallestIndex = index * 2 + 1; // left child
		else smallestIndex = index * 2 + 2; // right child
		
		if (arr[index]->freq > arr[smallestIndex]->freq) // exchange (for maxheap)
		{
			tNode* temp = arr[index];
			arr[index] = arr[smallestIndex];
			arr[smallestIndex] = temp;
			
			index = smallestIndex;
			
			noright = 0;
		}
		else return;
	}
}

// 최소값 제거
// heapDelete 함수 호출
tNode *heapDelete( HEAP *heap) {
    if (heap->last == -1) return NULL; // empty heap
	
	tNode *data = heap->heapArr[0];
	heap->heapArr[0] = heap->heapArr[heap->last];
	
	(heap->last)--;
	
	_reheapDown(heap, 0);

    return data;
}

// 힙 메모리 해제
void heapDestroy( HEAP *heap) {
	free(heap->heapArr);
	free(heap);
}

// 문자별 허프만 트리를 생성
// capacity 256 짜리 빈(empty) 힙 생성
// 개별 알파벳에 대한 노드 생성
// 힙에 삽입 (minheap 구성)
// 2개의 최소값 추출
// 결합 후 새 노드에 추가
// 힙에 삽입
// 힙에 한개의 노드가 남을 때까지 반복
tNode *make_huffman_tree( int *ch_freq) {
	int i;
	HEAP *huffmanHeap = heapCreate(256);

	for (i = 0; i < 256; i++) {
		tNode *node = newNode(i, ch_freq[i]);
		heapInsert(huffmanHeap, node);
	}

	tNode *node1 = NULL, *node2 = NULL, *tmpNode;
	for (i = 0; i < 255; i++) {
		node1 = heapDelete(huffmanHeap);
		node2 = heapDelete(huffmanHeap);
		tNode* tmpNode = newNode(0, node1->freq + node2->freq);
		tmpNode->left = node1;
		tmpNode->right = node2;
		heapInsert(huffmanHeap, tmpNode);
	}

	tNode *topNode = huffmanHeap->heapArr[0];
	heapDestroy(huffmanHeap);

	return topNode;
}

// 허프만 트리를 순회하며 허프만 코드를 생성하여 codes에 저장
// leaf 노드에서만 코드를 생성
// strdup 함수 사용함
void traverse_tree( tNode *root, char *code, int depth, char *codes[]) {
	if (root->left || root->right) {
		code[depth] = '0';
		traverse_tree(root->left, code, depth + 1, codes);

		code[depth] = '1';
		traverse_tree(root->right, code, depth + 1, codes);
	} else {
		codes[root->data] = strdup(code);
	}
	code[depth] = 0;
}

// 허프만 트리로부터 허프만 코드를 생성
// traverse_tree 함수 호출
void make_huffman_code( tNode *root, char *codes[]) {
	char code[256] = {0, };
	traverse_tree(root, code, 0, codes);
}

// 트리 메모리 해제
void destroyTree( tNode *root) {
	if (root->left) destroyTree(root->left);
	if (root->right) destroyTree(root->right);
	free(root);
}

////////////////////////////////////////////////////////////////////////////////
// 힙의 내용 출력 (for debugging)
void heapPrint( HEAP *heap)
{
	int i;
	tNode **p = heap->heapArr;
	int last = heap->last;
	
	for( i = 0; i <= last; i++)
	{
		printf("[%d]%c(%6d)\n", i, p[i]->data, p[i]->freq);
	}
	printf( "\n");
}

////////////////////////////////////////////////////////////////////////////////
// 문자별 빈도 출력 (for debugging)
void print_char_freq( int *ch_freq)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		printf( "%d\t%d\n", i, ch_freq[i]); // 문자인덱스, 빈도
	}
}

////////////////////////////////////////////////////////////////////////////////
// 문자별 빈도를 이용하여 허프만 트리와 허프만 코드를 생성
// return value : 허프만 트리의 root node
tNode *run_huffman( int *ch_freq, char *codes[])
{
	tNode *root;
	
	root = make_huffman_tree( ch_freq);
	
	make_huffman_code( root, codes);
	
	return root;
}

////////////////////////////////////////////////////////////////////////////////
// 허프만 코드를 화면에 출력
void print_huffman_code( char *codes[])
{
	int i;
	
	for (i = 0; i < 256; i++)
	{
		printf( "%d\t%s\n", i, codes[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////
// argv[1] : 입력 텍스트 파일
// argv[2] : 바이너리 코드 (텍스트 파일; encoding 결과) 
// argv[3] : 출력 텍스트 파일 (decoding 결과)
int main( int argc, char **argv)
{
	FILE *fp;
	FILE *infp, *outfp;
	int ch_freq[256] = {0,}; // 문자별 빈도
	char *codes[256]; // 문자별 허프만 코드 (ragged 배열)
	tNode *huffman_tree; // 허프만 트리
	
	if (argc != 4)
	{
		fprintf( stderr, "%s input-file encoded-file decoded-file\n", argv[0]);
		return 1;
	}

	////////////////////////////////////////
	// 입력 텍스트 파일
	fp = fopen( argv[1], "rt");
	if (fp == NULL)
	{
		fprintf( stderr, "Error: cannot open file [%s]\n", argv[1]);
		return 1;
	}

	// 텍스트 파일로부터 문자별 빈도 저장
	// getc 함수 호출
	int num_bytes = read_chars( fp, ch_freq);

	fclose( fp);

	// 문자별 빈도 출력 (for debugging)
	//print_char_freq( ch_freq);
	
	// 허프만 코드/트리 생성
	huffman_tree = run_huffman( ch_freq, codes);
	
	// 허프만 코드 출력 (stdout)
	print_huffman_code( codes);

	////////////////////////////////////////
	// 입력: 텍스트 파일
	infp = fopen( argv[1], "rt");
	// 출력: 바이너리 코드
	outfp = fopen( argv[2], "wt");
	
	// 허프만코드를 이용하여 인코딩(압축)
	int encoded_bytes = encoding( codes, infp, outfp);

	// 허프만 코드 메모리 해제
	free_huffman_code( codes);
	
	fclose( infp);
	fclose( outfp);

	////////////////////////////////////////
	// 입력: 바이너리 코드
	infp = fopen( argv[2], "rt");
	// 출력: 텍스트 파일
	outfp = fopen( argv[3], "wt");

	// 허프만 트리를 이용하여 디코딩
	decoding( huffman_tree, infp, outfp);

	// 허프만 트리 메모리 해제
	destroyTree( huffman_tree);

	fclose( infp);
	fclose( outfp);

	////////////////////////////////////////
	printf( "# of bytes of the original text = %d\n", num_bytes);
	printf( "# of bytes of the compressed text = %d\n", encoded_bytes);
	printf( "compression ratio = %.2f\n", ((float)num_bytes - encoded_bytes) / num_bytes * 100);
	
	return 0;
}


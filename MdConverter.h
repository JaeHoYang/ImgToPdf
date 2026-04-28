#pragma once
#include "pch.h"

/**
 * @brief Markdown 텍스트를 HTML 또는 RTF 로 변환하는 유틸리티 클래스.
 *
 * 외부 pandoc 등 의존 없이 C++ 로 직접 Markdown 을 파싱하여 변환한다.
 *
 * 지원 Markdown 문법:
 * - 제목: H1~H6 (#~######)
 * - 강조: **bold**, *italic*, ***bold italic***
 * - 인라인 코드: `code`
 * - 펜스 코드 블록: ``` ... ```
 * - 인용구: > blockquote
 * - 순서 없는 목록: - item, * item
 * - 순서 있는 목록: 1. item
 * - 구분선: ---
 * - 링크: [text](url)
 * - 이미지: ![alt](url)
 * - 단락: 빈 줄 구분
 */
class MdConverter
{
public:
    /**
     * @brief Markdown 텍스트를 완전한 독립 HTML 문서로 변환한다.
     *
     * 반환되는 HTML 은 \<html\>\<head\>\<body\> 를 모두 포함하며,
     * 인라인 CSS 스타일이 포함된 자기완결 문서이다.
     *
     * @param mdText Markdown 원문 텍스트
     * @param title  HTML \<title\> 태그에 사용할 문서 제목 (비워두면 빈 제목)
     * @return 완전한 HTML 문서 문자열
     */
    static CString ToHtml(const CString& mdText, const CString& title = _T(""));

    /**
     * @brief Markdown 텍스트를 RTF 바이트 문자열로 변환한다.
     *
     * 반환된 문자열은 CRichEditCtrl::StreamIn(SF_RTF, ...) 에 직접 사용할 수 있다.
     * 한국어 및 비ASCII 문자는 RTF \\uN? 이스케이프로 인코딩된다.
     *
     * @param mdText Markdown 원문 텍스트
     * @return RTF 형식의 바이트 문자열 (SF_RTF StreamIn 호환)
     */
    static std::string ToRtf(const CString& mdText);
};
